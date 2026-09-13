#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int res1 = system(".\\Debug\\jag.exe --version");
    if (res1 != 0) res1 = system(".\\Release\\jag.exe --version");
    if (res1 != 0) res1 = system(".\\jag.exe --version");
    if (res1 != 0) res1 = system("./jag --version");
    if (res1 != 0) res1 = system("jag --version");
    assert(res1 == 0);

    int res2 = system(".\\Debug\\jag.exe --help");
    if (res2 != 0) res2 = system(".\\Release\\jag.exe --help");
    if (res2 != 0) res2 = system(".\\jag.exe --help");
    if (res2 != 0) res2 = system("./jag --help");
    if (res2 != 0) res2 = system("jag --help");
    assert(res2 == 0);

    printf("CLI tests passed successfully!\n");
    return 0;
}
