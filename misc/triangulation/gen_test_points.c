
#include <stdio.h>

int
main(int argc, char ** argv)
{
    int NUM;
    int i, x, y, z;
    srandom(getpid() * time(0));

    if (argc != 2)
    {
    usage:
        printf("gen_test_points <num>\n");
        return 1;
    }

    NUM = atoi(argv[1]);

    printf("/* this file autogenerated by gen_test_points.c */\n");
    printf("#define NUM_COORDS %d\n", NUM);
    printf("Point test_points[%d] = {\n", NUM);

    for (i=0; i < NUM; i++)
    {
        x = random() % 100;
        y = random() % 100;
        z = random() % 100;
        
        printf("    { %d.%d, %d.%d, %d.%d }%s\n",
               x/10, x%10, y/10, y%10, z/10, z%10,
               (i != (NUM-1)) ? "," : "");
    }

    printf("};\n");
    return 0;
}
