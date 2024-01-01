#include <stdio.h>
#include <xutils/file_map.h>

int
main(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
        printf("%s: %ld\n", argv[i], devsize(argv[i]));
    if (argc == 1)
        printf("No devices\n");
    return (0);
}
