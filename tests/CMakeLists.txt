add_executable(test_regexjit
    test_parser.cpp
    test_compiler.cpp
    test_match.cpp
    test_find.cpp
    test_substitute.cpp
    test_advanced_pcre.cpp
)

target_link_libraries(test_regexjit
    PRIVATE
        regexjit
        Boost::unit_test_framework
)

add_test(NAME RegexJitTests COMMAND test_regexjit)
