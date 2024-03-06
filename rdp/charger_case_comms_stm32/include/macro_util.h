#include <stdint.h>

// -------------------- general macros -------------------

#define BINARY_OP_COMBINATION(OP, __0, __1, __2, __3, __4, __5, __6, __7, __8, __9, _10, _11, \
                              _12, _13, _14, _15, ...)                                        \
    (__0 OP __1 OP __2 OP __3 OP __4 OP __5 OP __6 OP __7 OP __8 OP __9 OP _10 OP _11 OP _12  \
         OP _13 OP _14 OP _15)

#define MAP(COMBINATION_OP, OP, __0, __1, __2, __3, __4, __5, __6, __7, __8, __9, _10, _11,   \
            _12, _13, _14, _15, ...)                                                          \
    (COMBINATION_OP(OP(__0), OP(__1), OP(__2), OP(__3), OP(__4), OP(__5), OP(__6), OP(__7),   \
                    OP(__8), OP(__9), OP(_10), OP(_11), OP(_12), OP(_13), OP(_14), OP(_15)))

#define MAP_PAIRS(COMBINATION_OP, PAIRWISE_OP, __0a, __0b, __1a, __1b, __2a, __2b, __3a,      \
                  __3b, __4a, __4b, __5a, __5b, __6a, __6b, __7a, __7b, __8a, __8b, __9a,     \
                  __9b, _10a, _10b, _11a, _11b, _12a, _12b, _13a, _13b, _14a, _14b, _15a,     \
                  _15b, ...)                                                                  \
    COMBINATION_OP(PAIRWISE_OP(__0a, __0b), PAIRWISE_OP(__1a, __1b), PAIRWISE_OP(__2a, __2b), \
                   PAIRWISE_OP(__3a, __3b), PAIRWISE_OP(__4a, __4b), PAIRWISE_OP(__5a, __5b), \
                   PAIRWISE_OP(__6a, __6b), PAIRWISE_OP(__7a, __7b), PAIRWISE_OP(__8a, __8b), \
                   PAIRWISE_OP(__9a, __9b), PAIRWISE_OP(_10a, _10b), PAIRWISE_OP(_11a, _11b), \
                   PAIRWISE_OP(_12a, _12b), PAIRWISE_OP(_13a, _13b), PAIRWISE_OP(_14a, _14b), \
                   PAIRWISE_OP(_15a, _15b))

#define UNION(...)                          BINARY_OP_COMBINATION(&, __VA_ARGS__)
#define XOR(...)                            BINARY_OP_COMBINATION(^, __VA_ARGS__)
#define OR(...)                             BINARY_OP_COMBINATION(|, __VA_ARGS__)

#define VARIABLE_ARG_INDIRECTION(FUNC, ...) FUNC(__VA_ARGS__)
