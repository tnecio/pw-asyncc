include_directories(..)

add_executable(test_defer defer.c)
add_test(test_defer test_defer)

add_executable(test_await await.c)
add_test(test_await test_await)

add_executable(test_queue test_queue.c)
add_test(test_queue test_queue)

set_tests_properties(test_defer test_await PROPERTIES TIMEOUT 1)