#include "jag/runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    JagVal v1 = jag_val_num(33);
    assert(v1.kind == JAG_VAL_NUM);

    JagVal v2 = jag_val_string("Hello Jaguar");
    assert(v2.kind == JAG_VAL_STRING);

    JagArray *arr = jag_array_create(4);
    jag_array_append(arr, v1);
    jag_array_append(arr, v2);
    assert(arr->count == 2);

    jag_array_free(arr);
    free(v2.as.string);

    printf("Runtime tests passed successfully!\n");
    return 0;
}
