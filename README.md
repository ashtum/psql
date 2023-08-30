## Psql

**Psql** is an asynchronous C++ PostgreSQL client based on Boost.Asio and libpq.

> [!WARNING]  
> This is currently a work in progress and is not yet a production-ready library.

### Quick usage

**Note:** This library conforms to Asio universal asynchronous model, which means it can be used with callbacks, coroutines, futures, Boost.Fiber and any other form of completion tokens.


#### Connecting to the database

You can create a `psql::connection` by providing an executor instance; afterward, initiate an asynchronous connection operation by calling async_connect with a [connection URI](https://www.postgresql.org/docs/15/libpq-connect.html#LIBPQ-CONNSTRING) along with the desired completion token.

```C++
auto exec = co_await asio::this_coro::executor;
auto conn = psql::connection{ exec };

co_await conn.async_connect("postgresql://localhost:5433", asio::deferred);
```
Related example: [simple.cpp](example/simple.cpp)


Alternatively, you can utilize a `psql::connection_pool` to efficiently acquire and recycle connections.

```C++
auto exec = co_await asio::this_coro::executor;
// Create a connection_pool with a max_size of 64.
auto conn_pool = psql::connection_pool{ exec, conninfo, 64 };

// Acquire a connection from the connection pool.
// If the number of acquired connections reaches the max_size, this operation
// will wait until a connection is returned to the connection pool.
auto conn = co_await conn_pool.async_aquire(asio::deferred);

// The destructor of pooled_connection will automatically return the connection
// to the connection pool.
```
Related example: [connection_pool.cpp](example/connection_pool.cpp)


#### Performing queries

You can use `async_query` to initiate an asynchronous query. the completion token completes with an instance of `psql::result` which can be discarded when the result is not needed.

```C++
co_await conn.async_query("CREATE TABLE actors (name TEXT, age INT);", asio::deferred);
```
Related example: [simple.cpp](example/simple.cpp)


#### Iterating through the rows in a `psql::result`

You can utilize `psql::as` on instances of `psql::result`, `psql::row`, and `psql::field` to deserialize the fields into your preferred types.  
**Note:** because of C++ ADL, using the psql namespace before `as` is unnecessary.  
**Note:** `as` throws an exception if the field types do not match the provided type or if the number of fields is fewer than expected.  

```C++
auto actors = co_await conn.async_query("SELECT name, age FROM actors", asio::deferred);
for (const auto row : actors)
{
  // Because result is preserved in the `actors` variable, we can use std::string_view for accessing TEXT fields.
  const auto [name, age] = as<std::string_view, int>(row);
  std::cout << name << ": " << age << std::endl;

  // Alternatively:
  std::cout << as<std::string_view>(row.at("name")) << ": " << as<int>(row.at("age"))  << std::endl;
  // or:
  std::cout << as<std::string_view>(row.at(0)) << ": " << as<int>(row.at(1)) << std::endl;
}
```
Related example: [simple.cpp](example/simple.cpp)


#### Converting single row results directly

```C++
// Here we use std::string for deserializing the first field because in this scenario,
// the result of async_query would be a temporary object. If we had used std::string_view,
// it could point to a destroyed buffer.
auto [a, b] = as<std::string, int>(co_await conn.async_query("SELECT 'one'::TEXT, 2;", asio::deferred));
std::cout << a << "-" << b << std::endl;
```
Related example: [simple.cpp](example/simple.cpp)


#### Passing query parameters

`psql::mp` can be used for constructing `psql::params` instances.

```C++
co_await conn.async_query("INSERT INTO actors VALUES ($1, $2);", psql::mp("Bruce Lee", 32), asio::deferred);
```

PostgreSQL permits the definition of a column as an array of any valid data type, including built-in types, user-defined types, or enumerated types.
```C++
auto result = co_await conn.async_query("SELECT $1;", psql::mp(std::array{ "1", "2", "3" }), asio::deferred);

// We can deserialize array fields into sequential containers.
for (const auto value : as<std::vector<std::string_view>>(result))
  std::cout << value << ' ';
```
Related example: [simple.cpp](example/simple.cpp)


#### Pipeline mode

A pipeline is beneficial when we need to dispatch a series of queries and await their results. For example, a batch of INSERTs or SELECTs.  
**Note:** Pipelines are implicit transactions, that means operations that have already executed are rolled back and operations that were queued to follow the failed operation are skipped entirely.
```C++
auto results = co_await conn.async_exec_pipeline(
[](psql::pipeline& p)
{
  p.push_query("DROP TABLE IF EXISTS phonebook;");
  p.push_query("CREATE TABLE phonebook(phone TEXT, name TEXT);");
  p.push_query("INSERT INTO phonebook VALUES ($1, $2);", psql::mp("+1 111 444 7777", "Jake"));
  p.push_query("INSERT INTO phonebook VALUES ($1, $2);", psql::mp("+2 333 222 3333", "Amie"));
  p.push_query("SELECT * FROM phonebook ORDER BY name;");
},
asio::deferred);

// We use the last item in the vector, which represents the outcome of the SELECT query.
for (const auto row : results.back())
{
  const auto [phone, name] = as<std::string_view, std::string_view>(row);
  std::cout << name << ":" << phone << std::endl;
}
```
Related example: [pipeline.cpp](example/pipeline.cpp)


#### Prepared statements

`async_prepare` can be used to create a prepared statement for later execution with `async_query_prepared`.
``` C++
co_await conn.async_prepare("add_two", "SELECT $1::INT + $2::INT;", asio::deferred);

auto result = co_await conn.async_query_prepared("add_two", psql::mp(1, 2), asio::deferred);
std::cout << as<int>(result) << std::endl;

// A pipeline of prepared queries.
auto results = co_await conn.async_exec_pipeline(
  [](psql::pipeline& p)
  {
    for (auto i = 0; i < 10; i++)
      p.push_query_prepared("add_two", psql::mp(1, i));
  },
  asio::deferred);
```
Related example: [prepared_statements.cpp](example/prepared_statements.cpp)


#### User defined types

User-defined types can be mapped to their types in the PostgreSQL server by specializing `psql::user_defined<T>` class.
* `user_defined<T>::name` will be used to query Oid of the type from PostgreSQL server.
* `user_defined<T>::members` must be a tuple of member varaible pointers.
* User-defined types can also encompass other user-defined types or arrays.

``` C++
struct Employee
{
  std::string name;
  std::string phone;
};

struct Company
{
  std::int64_t id;
  std::vector<Employee> employees;
};

namespace psql
{
template<>
struct user_defined<Employee>
{
  static constexpr auto name    = "employee";
  static constexpr auto members = std::tuple{ &Employee::name, &Employee::phone };
};

template<>
struct user_defined<Company>
{
  static constexpr auto name    = "company";
  static constexpr auto members = std::tuple{ &Company::id, &Company::employees };
};
}

// Usage:
auto company = Company{ 104, { { "Jane Eyre", "555-123-4567" }, { "Tom Hanks", "555-987-6543" } } };

// The connection will query the Oid of user-defined types through the provided name
// in the specialization of user_defined<>. These Oids will be stored within the
// connection, eliminating the need for future queries to retrieve Oid values.
auto result = co_await conn.async_query("SELECT $1;", psql::mp(company), asio::deferred);

// Deserializes the result as a user-defined type.
auto [id, employees] = as<Company>(result);
```
Related example: [user_defined.cpp](example/user_defined.cpp)


#### Notification

You can use `async_receive_notifcation` to receive notifications which completes with an instance of `psql::notification`.  

* `async_receive_notification` is the only asynchronous operation that can be initiated while other active asynchronous operations are ongoing on the connection.  
* When there is no active async_receive_notification operation, received notifications will accumulate in the connection and can be read with future calls.  

```C++
co_await conn.async_query("LISTEN counter;", asio::deferred);

auto notif = co_await conn.async_receive_notifcation(asio::deferred);

std::cout << "Channel:" << notif.channel() << "\tPayload:" << notif.payload() << std::endl;
```
Related example: [notification.cpp](example/notification.cpp)
