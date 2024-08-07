# Concurrency Library 
В этой библиотеке представлены выразительные средства (stackful корутины и файберы (user space потоки) со способами для их прерывания) и среды исполнения для Concurrency на C++.  (Содержимое библиотеки лежит в [`execution`](execution/))

Concurrency декомпозирована:
- _Выразительные средства_ _описывают_ цепочки задач
- _Среда исполнения_ _исполняет_ эти цепочки

# Структура `exe`

- [`coro`](execution/exe/coro) – корутины
- [`executors`](execution/exe/executors) – экзекуторы (планировщики задач)
  - `thread_pool` - Стандартный пул потоков
  - `strand` - Асинхронный мьютекс
  - `manual` - Однопоточный экзекутор
- [`fibers`](execution/exe/fibers) – файберы, зависят от `IExecutor` и `coro`
  - `sched` – системные вызовы к планировщику (запуск и прерывание)
- [`threads`](execution/exe/threads) – синхронизация для потоков (Spinlock, QueueSpinlock, etc.)
- [`support`](execution/exe/support) – полезные мелочи (Lock-free очереди и стэк, блокирующие очереди, вейтгруппа)

Пользователь библиотеки работает только с
- `executors` и
- `fibers/sched`

Ниже приведено описание основных сущностей.

# [Stackful] Coroutine

_Сопрограмма_ (_coroutine_) или просто _корутина_ – это процедура,
из вызова которой можно выйти в середине (_остановить_ вызов, _suspend_), а затем вернуться в этот вызов
и продолжить исполнение с точки остановки.

Экземпляр класса `Coroutine` представляет вызов процедуры,
1) который может остановиться (операция `Suspend`), и
2) который затем можно возобновить (операция `Resume`).

Создадим корутину:

```cpp
Coroutine co(routine);
```

Здесь `routine` – произвольная пользовательская процедура, которая будет исполняться корутиной `co`. 

Непосредственно создание корутины не приводит к запуску `routine`.

Созданная корутина запускается вызовом `co.Resume()`. После этого управление передается процедуре `routine`, и та исполняется до первого вызова `Suspend()` (или до своего завершения).

Вызов `Suspend()` в корутине останавливает ее исполнение, передает управление обратно caller-у и завершает его вызов `co.Resume()`. Вызов `Suspend()` – это точка выхода из корутины, _suspension point_.

Следующий вызов `co.Resume()` вернет управление остановленной корутине, вызов `Suspend()` в ней завершится, и она продолжит исполнение до очередного `Suspend()` или же до завершения своей процедуры.

С исключениями корутины взаимодействуют как обычные функции: если в корутине было выброшено и не перехвачено исключение, то оно вылетит в caller-е из вызова `co.Resume()` и полетит дальше (выше) по цепочке вызовов.

### Пример

```cpp
void StackfulCoroutineExample() {
  // Stackful корутина
  Coroutine co([] {
    // Роль: callee
    
    fmt::println("Step 2");
    
    Coroutine::Suspend();  // <-- Suspension point

    fmt::println("Step 4");
  });
  // <-- Исполнение процедуры `routine` пока не стартовало

  fmt::println("Step 1");

  // Роль: caller
  // Стартуем корутину
  // Управление передается процедуре routine,
  // и та исполняется до первого вызова Suspend().
  co.Resume();

  fmt::println("Step 3");

  co.Resume();

  // Вызов IsCompleted() возвращает true если корутина уже завершила 
  // свое исполнение (дойдя до конца `routine` или бросив исключение).
  assert(co.IsCompleted());

  // Попытка вызова Suspend вне корутины - UB
  // Coroutine::Suspend()
}
```

На экране будет напечатано:
```
Step 1
Step 2
Step 3
Step 4
```

В данной библиотеке приведена _stackful_ реализация корутины: в вызове `Suspend` корутина сохраняет текущий контекст исполнения и активирует контекст исполнения родителя, останавливая тем самым весь текущий стек вызовов, прыгая на другой стек. (Также существует [stackless](https://en.cppreference.com/w/cpp/language/coroutines) подход, но он требует поддержки со стороны компилятора).

### Применения корутин
Корутины можно использовать не только в реализации Файберов, но и сами по себе: [`SimpleCoroutine`](execution/exe/coro/simple.hpp), например для реализации итераторов и генераторов.

# Файберы

Файберы – это кооперативная многозадачность: независимые активности, чередующиеся на процессоре.

За исполнение файберов отвечает планировщик, его задача – распределять файберы между потоками (аналогично планировщику операционной системы, который распределяет потоки между ядрами процессора) и при остановке файбера выбирать следующий файбер ему на замену.

Файбер по сути комбинирует в себе планировщик и stackful корутины.

## Пример

```cpp
void FibersExample() {
  using namespace exe;

  // Пул потоков будет служить планировщиком для файберов
  executors::ThreadPool scheduler{/*threads=*/4};
  scheduler.Start();

  for (size_t i = 0; i < 256; ++i) {
    // Запускаем файбер в пуле-планировщике
    fibers::Go(scheduler, [] {
      for (size_t j = 0; j < 3; ++j) {
        // Уступаем поток планировщика другому файберу
        fibers::Yield();
      }  
    });
  }
  
  // Файберы исполняются _параллельно_ на разных потоках пула-планировщика

  // Дожидаемся завершения файберов
  scheduler.WaitIdle();
  // Выключаем планировщик
  scheduler.Stop();
}  
```

- `Go(Scheduler&, Routine)` – запланировать процедуру на исполнение в файбере в заданный планировщик
- `Go(Routine)` – запланировать процедуру на исполнение в файбере в текущий планировщик
- `Yield` – перепланировать текущий файбер, уступить поток планировщика другим файберам

## Замечания по реализации

Файберы аллоцируются на куче и сами себя удаляют по окончании исполнения.

Файберы реализуют `TaskBase`, позволяющая использовать интрузивные очереди в планировщике (т.е. не происходит лишних аллокаций + это лучше соответствует дизайну файбера как Task'и в планировщике)


# Executors

В библиотеке абстрагируется среда исполнения: файберы и корутины будут запускать свои задачи не в конкретном пуле потоков, а в абстрактном _экзекуторе_.

_Экзекутор_ (_executor_) – это сервис, исполняющий задачи. 

Экзекутор реализует интерфейс [`IExecutor`](execution/exe/executors/executor.hpp) с единственным методом `Submit` – запланировать задачу на исполнение. Механика исполнения задач скрыта от пользователя за виртуальным вызовом.

## Дизайн

- _Выразительные средства_ (фьючи, файберы, корутины) описывают _что_ исполнять
- _Экзекуторы_ (`ThreadPool`, `ManualExecutor`, `Strand`) – _как_ исполнять
- `IExecutor` – граница, разделяющая выразительные средства и среду исполнения задач, позволяющая их комбинировать

## Реализации

### Пул потоков

[`ThreadPool`](execution/exe/executors/tp/compute/thread_pool.hpp) – статический набор потоков-воркеров, разбирающих общую очередь задач.

```cpp
void ThreadPoolExample() {
  using namespace exe;
  
  // ThreadPool реализует интерфейс IExecutor
  executors::ThreadPool pool{/*threads=*/4};
  pool.Start();
  
  executors::Submit(pool, [] {
    fmt::println("Running on thread pool");
  });
  
  // Дожидаемся завершения всех задач
  pool.WaitIdle();
  // Останавливаем пул
  pool.Stop();
}
```

### Strand

#### Проблема

Задачам в пуле потоков может понадобиться взаимное исключение.

Использовать непосредственно мьютексы нежелательно: если задача в пуле захватила мьютекс на продолжительное время, то другие задачи, которым нужен этот же мьютекс, заблокируют потоки пула и помешают исполняться другим задачам, которым этот мьютекс вообще не нужен.

#### Асинхронный мьютекс

Экзекуторы предоставляют альтернативу: _асинхронный мьютекс_ или `Strand`.

[`Strand`](execution/exe/executors/strand.hpp) – экзекутор, запускающий задачи (можно назвать их критическими секциями) строго **последовательно**, в порядке их планирования (вообще говоря, этот порядок частичный – _happens-before_).

Планировать задачи в `Strand` можно из разных потоков без внешней синхронизации.

`Strand` не имеет собственных потоков, это декоратор, который оборачивает произвольный экзекутор и делегирует тому исполнение своих задач.

#### Пример

```cpp
void StrandExample() {
  using namespace exe::executors;
  
  // Потоки, исполняющие задачи
  ThreadPool workers{4};
  // Декоратор над пулом потоков `workers`
  // Сам `strand` не знает, что он декорирует именно пул потоков
  Strand strand{workers};
  
  ThreadPool clients{5};
  
  size_t cs = 0;
  
  // Запускаем в пуле `clients` задачи, которые будут
  // параллельно планировать критические секции в `strand`
  
  for (size_t i = 0; i < 1024; ++i) {
    Submit(clients, [&] {
      // Следующий `Submit` вызывается из разных потоков пула `clients`
      Submit(strand, [&] {
        // Асинхронная критическая секция
        ++cs;
      });
    });
  }
  
  // Ждем завершения клиентов
  clients.WaitIdle();
  // Ждем завершения секций
  workers.WaitIdle();
  
  fmt::println("# critical sections: {}", cs);
  // <-- Напечатано 1024
  
  clients.Stop();
  workers.Stop();
}
```

#### `Strand` и мьютекс

С точки зрения протокола когерентности кэшей, синхронный мьютекс – неэффективный примитив синхронизации:

Чем больше число ядер / потоков, исполняющих критические секции, тем выше накладные расходы на когерентность, так как разделяемые данные, к которым обращаются критические секции, приходится постоянно двигать между ядрами.

Гораздо разумнее не данные "двигать" между ядрами к критическим секциям, а наоборот – критические секции собрать на одном ядре, и на нем обрабатывать данные. Тогда секции будут работать над "прогретым" кэшом.

Это и делает `Strand`. 

Чем больше будет нагрузка на `Strand`, тем **меньше** будет в исполнении синхронизации, и тем эффективнее будут исполняться критические секции пользователей (в отличие от обычного мьютекса, в котором все наоборот)!

`Strand` исполняет задачи _сериями_ или _пакетами_ (_batching_).

1) Серийность позволит избавиться от синхронизации между критическими секциями при исполнении серии.
2) Критические секции внутри серии будут работать над прогретым кэшом.

В библиотеке `Strand` реализован через лок-фри очередь


### Manual

[`ManualExecutor`](execution/exe/executors/manual.hpp) – это всего лишь очередь задач.

Вызов `Submit` добавляет задачу в конец этой очереди.

Задачи, запланированные в `ManualExecutor`, запускаются **вручную** с помощью методов `RunNext`, `RunAtMost` и `Drain`. 

Собственных потоков у `ManualExecutor` нет.

С `ManualExecutor` должен работать только один поток.

#### Пример

```cpp
void ManualExample() {
  using namespace exe;
  
  // ManualExecutor – очередь задач
  executors::ManualExecutor manual;
  
  // Добавляем задачу в очередь
  executors::Submit(manual, [] {
    fmt::println("1st");
  });
  
  // И еще одну
  executors::Submit(manual, [] {
    fmt::println("2nd");
  });

  // <-- Теперь обе задачи находятся в очереди ManualExecutor-а,
  // пока ни одна из них не была запущена

  // Запустим первую задачу
  manual.RunNext();

  // Запланируем третью задачу
  executors::Submit(manual, [] {
    fmt::println("3rd");
  });

  // "Опустошаем" очередь задач, т.е.
  // исполняем задачи до тех пор, пока очередь не опустеет
  manual.Drain();
  // <-- К этому моменту выполнились все три задачи
}
```