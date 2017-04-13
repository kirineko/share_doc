#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/sem.h>

/* 全局变量 */

int numOfEmptyPlate_sem_id = 0; //盘子的数量（盘子里装2个水果，相当于2个空盘子）
int numOfApple_sem_id = 0; //苹果的数量
int numOfOrange_sem_id = 0; //橘子的数量
int getAccesstoPlate = 0; //访问盘子的信号量

// 2 PLATES，这个数组用来表示2个空盘子，plate[0] = 0, 表示第一个盘子为空， plate[0] = 1, 表示第一个盘子装了苹果，plate[0] = 2 表示橘子
int *plate = NULL;

struct sembuf P, V;    // P,V原语



void printPlates(const char *arg)
{
    int i = 0;
    printf("%s\n盘子里的水果是:", arg);

    for(i=0; i<2; i++)
    {
        printf("%d ", plate[i]);
    }
    printf("\n");
}


/**
 * 父亲放水果
 * 1 == apple
 * 2 == orange
 */
void father_do()
{
    int i = 0;
    semop(numOfEmptyPlate_sem_id, &P, 1); // 放水果需要占用1个盘子

    //  rand()%100产生一个[0,99]的随机数
    //  如果这个随机数大于50，就放橘子，反之就放苹果
    if(rand()%100+1 > 50)  //放橘子
    {
        sleep(1);  //父亲花了1s的时间放橘子
        semop(getAccesstoPlate, &P, 1); //放水果需要占用盘子的访问权，用P操作占用盘子的访问权
        printf("父亲得到一个盘子.\n");
        printPlates("父亲:");

        //父亲遍历2个盘子，寻找一个空盘子(=0)，然后在这个盘子中放橘子(=2)
        for(i=0; i<2; i++)
        {
            if(0 == plate[i])
            {
                printf("%d ", plate[i]);
                plate[i] = 2;
                break;
            }
        }

        semop(numOfOrange_sem_id, &V, 1); //父亲放完橘子，相当于父亲释放了一个橘子
        printf("父亲放了一个橘子.\n");
        semop(getAccesstoPlate, &V, 1); //父亲放完橘子，用V操作释放对盘子的访问权
        printf("父亲归还盘子\n");
    }
    else   //放苹果
    {
        sleep(1); //父亲花了1s时间放苹果
        semop(getAccesstoPlate, &P, 1); //放水果需要占用盘子的访问权，用P操作占用盘子的访问权
        printf("父亲得到一个盘子.\n");
        printPlates("父亲:");

        //父亲遍历2个盘子，寻找一个空盘子(=0)，然后在这个盘子中放橘子(=1)
        for(i=0; i<2; i++)
        {
            if(0 == plate[i])
            {
                printf("%d ", plate[i]);
                plate[i] = 1;
                break;
            }
        }

        semop(numOfApple_sem_id, &V, 1);  //父亲放完苹果，相当于父亲释放了一个苹果
        printf("父亲放了一个苹果\n");
        semop(getAccesstoPlate, &V, 1); //父亲放完苹果，用V操作释放对盘子的访问权
        printf("父亲归还盘子\n");
    }
}

/**
 * 女儿吃橘子
 */
void girl_do()
{
    int i = 0;
    semop(numOfOrange_sem_id, &P, 1); //女儿吃橘子，相当于占用一个橘子

    sleep(1); //女孩花1s吃橘子

    semop(getAccesstoPlate, &P, 1); //女儿取橘子，需要占用盘子的得访问权
    printf("女儿控制一个盘子\n");
    printPlates("女儿:");

    //女儿遍历2个盘子，寻找一个有橘子的盘子(=2)，然后在吃掉橘子(=0)
    for( i=0; i<2; i++)
    {
        if( 2 == plate[i] )
        {
            printf("%d ", plate[i]);
            plate[i] = 0;
            break;
        }
    }

    semop( numOfEmptyPlate_sem_id, &V, 1); //吃完橘子，用V操作释放1个盘子
    printf("女儿吃橘子\n");
    semop( getAccesstoPlate, &V, 1); //吃完橘子，需要释放对盘子的访问权
    printf("女儿归还盘子.\n");
}


/**
 * 儿子吃苹果
 */
void boy_do()
{
    int i = 0;
    semop( numOfApple_sem_id, &P, 1); //儿子吃苹果，相当于占用一个苹果

    sleep(1); //儿子花1s吃苹果

    semop( getAccesstoPlate, &P, 1); //儿子取苹果，需要占用盘子的得访问权
    printf("儿子控制一个盘子.\n");
    printPlates("儿子:");

    //儿子遍历2个盘子，寻找一个有苹果的盘子(=1)，然后在吃掉苹果(=0)
    for( i=0; i<2; i++)
    {
        if( 1 == plate[i] )
        {
            printf("%d ", plate[i]);
            plate[i] = 0; // eat an apple
            break;
        }
    }

    semop( numOfEmptyPlate_sem_id, &V, 1); // 吃完苹果，用V操作释放1个盘子
    printf("儿子吃苹果.\n");
    semop( getAccesstoPlate, &V, 1); //吃完苹果，需要释放对盘子的访问权
    printf("儿子归还盘子.\n");
}



int main()
{
    /**
     * 3个进程的ID
     */
    pid_t father_id;
    pid_t girl_id;
    pid_t boy_id;

    char op =' ';
    srand(time(0)); //根据时间生成一个随机数

    plate=(int *)mmap(NULL, sizeof(int)*2, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS,-1,0); //取得一个共享内存，用来表示盘子
    plate[0] = 0; //初始化第一个盘子为空（盘子的第一个位置为NULL)
    plate[1] = 0; //初始化第二个盘子为空（盘子的第二个位置为NULL)

    /**
     * 使用semget创建4个信号量集对象
     */
    numOfApple_sem_id = semget( IPC_PRIVATE, 1, IPC_CREAT|00666);
    numOfEmptyPlate_sem_id = semget( IPC_PRIVATE, 1, IPC_CREAT|00666);
    numOfOrange_sem_id = semget( IPC_PRIVATE, 1, IPC_CREAT|00666);
    getAccesstoPlate = semget( IPC_PRIVATE, 1, IPC_CREAT|00666);



    //设置信号量集中信号量的值
    if( -1 == semctl(numOfApple_sem_id, 0, SETVAL, 0) ||
            -1 == semctl(numOfEmptyPlate_sem_id, 0, SETVAL, 2) ||
            -1 == semctl(numOfOrange_sem_id, 0, SETVAL, 0)  ||
            -1 == semctl(getAccesstoPlate, 0, SETVAL, 1) )
    {
        perror("Semctl serval Error!\n");
    }

    //初始化信号量PV原语
    V.sem_num = 0;
    V.sem_op = 1;
    V.sem_flg = SEM_UNDO;

    P.sem_num = 0;
    P.sem_op = -1;
    P.sem_flg = SEM_UNDO;

    //用fork产生子进程
    father_id = fork();

    if( father_id < 0 )//Error occures
    {
        fprintf(stderr, "Fork father Fail.\n");
    }
    else if( 0 == father_id ) //如果father_id = 0，表示父进程
    {
        while(1) {
            father_do();
        }
    }
    else // 否则表示子进程
    {
        girl_id = fork(); //产生一个女儿进程

        if( girl_id <0 ) //Error occures
        {
            fprintf(stderr, "Fork gril Fail.\n");

        }
        else if( 0 == girl_id ) //用girl_id = 0 表示女儿进程
        {
            while(1) {
                girl_do();
            }
        }
        else //否则表示子进程
        {
            boy_id = fork(); //产生一个儿子进程
            if( boy_id < 0) //Error Occures
            {
                fprintf(stderr, "Fork boy Fail.\n");
            }
            else if ( 0 == boy_id) //用boy_id = 0 表示儿子进程
            {
                while(1) {
                    boy_do();
                }
            }
            else  //如果用户输入q，则退出模拟程序
            {
                do{
                    op= getchar();
                }while( op!='q');
                exit(0);
            }
        }
    }

    return 0;
}
