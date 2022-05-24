#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
    
    FILE *file1, *file2;
    int n, m, n2, m2;
    if (argc != 3)
    {
        printf("usage: addmx filename1 filename2\n");
        return EXIT_FAILURE;
    }

    file1 = fopen(argv[1], "r");
    file2 = fopen(argv[2], "r");

    char c;
    fscanf(file1, "%d %c %d", &n, &c, &m);
    fscanf(file2, "%d %c %d", &n2, &c, &m2);
    if (n != n2 || m != m2)
    {
        return EXIT_FAILURE;
    }
   
    int mx1[n*m], mx2[n*m];

    for (int i = 0; i < n * m; i++)
    {
        fscanf(file1, "%d", &mx1[i]);
        fscanf(file2, "%d", &mx2[i]);
        // printf("%d %d ",mx1[i],mx2[i]);
    }

    int *sum = mmap(NULL, n * m * sizeof(int), PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if (sum == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < m; i++)
    {
        pid_t pid;
        if ((pid = fork()) < 0)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid == 0)
        {
            for (int j = 0; j < n; j++)
            {
                sum[j * m + i] = mx1[j * m + i] + mx2[j * m + i];
            }
            exit(EXIT_SUCCESS);
        }
    }
    for (int i = 0; i < m; i++)
    {
        if (waitpid(-1, NULL, 0) < 0)
        {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
    }

    printf("%dx%d\n",n,m);
    for(int i=0;i<n;i++)
    {
        for(int j=0;j<m;j++)
        {
            printf("%d ",sum[i*m+j]);
        }
        printf("\n");
    }

    if (munmap(sum, sizeof(sum)) < 0)
    {
        perror("munmap");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}
