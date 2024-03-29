#include <stdio.h>
#include <assert.h>

struct Frame
{
    // PC pointer
    int pc;

    // arguments
    int arg;
};
typedef struct Frame Frame;

int f(int f)
{

    printf("f function.");

    return f;
}

int g(int g)
{
    printf("g functionl");

    return g;
}

int main(int argc, char **argv)
{
    


}