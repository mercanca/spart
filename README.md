# spart
A user-oriented partition info command for slurm.

Slurm does not have a command showing partition info in a user-friendly way.
I wrote a command, I hope you will find it useful. 

## Usage

**spart [-m] [-a] [-g] [-l] [-h]**


The output of spart without any parameters is as below:

```
$ spart
      QUEUE    FREE  TOTAL   FREE  TOTAL RESORC  OTHER    MIN    MAX MAXJOBTIME   CORES    NODE
  PARTITION   CORES  CORES  NODES  NODES PENDNG PENDNG  NODES  NODES  DAY-HR:MN PERNODE  MEM-GB
       defq*    168   2436      6     87      0   1120      1      -    7-00:00      28     128
     shortq     168   2604      6     93      0      0      1      2    0-01:00      28     128
      longq      24    336      1     14    144      0      1      -   21-00:00      24      64
       gpuq       0    112      0      4      0      0      1      -    7-00:00      28     128
    bigmemq       0    280      0     10      0      0      1      -    7-00:00      28     512
      b224q     168   2548      6     91      0      0      8     40    1-00:00      28     128
    core40q      40   1400      1     35    160      0      1      -    7-00:00      40     192
        all.    232   4340      8    142      0      0      1      -   NO-LIMIT      24      64
 ```
 In the **QUEUE PARTITION** column, the * . ! #  characters mean 'default queue', 'hidden queue', 'you can submit a job, but will not start', and 'you can not submit a job', repectively.
 
 The **RESOURCE PENDING** column shows the core count of pending jobs because of the busy resource. 

 The **OTHER PENDING** column shows the core count of pending jobs because of the other reasons such as license or other limits. 

 If **MIN NODES, MAX NODES, and MAXJOBTIME** limits are not setted for the all partitions in your cluster, corresponding column(s) will not be shown.

 The **CORES PERNODE** column shows the core count of the node with lowest core count in this partition.

 The **NODE MEM-GB** column shows the memory of the lowest memory node in this partition.

 If the **-m** parameter was given, both the lowest and highest values will be shown in the **CORES PERNODE** and **NODE MEM-GB** columns:

```
$ spart
      QUEUE    FREE  TOTAL   FREE  TOTAL RESORC  OTHER    MIN    MAX MAXJOBTIME   CORES    NODE
  PARTITION   CORES  CORES  NODES  NODES PENDNG PENDNG  NODES  NODES  DAY-HR:MN PERNODE  MEM-GB
       defq*    168   2436      6     87      0   1120      1      -    7-00:00      28 128-512
     shortq     168   2604      6     93      0      0      1      2    0-01:00      28 128-512
      longq      24    336      1     14    144      0      1      -   21-00:00      24      64
       gpuq       0    112      0      4      0      0      1      -    7-00:00      28 128-512
    bigmemq       0    280      0     10      0      0      1      -    7-00:00      28     512
      b224q     168   2548      6     91      0      0      8     40    1-00:00      28 128-512
    core40q      40   1400      1     35    160      0      1      -    7-00:00      40     192
        all.    232   4340      8    142      0      0      1      -   NO-LIMIT   24-40  64-512
 ```

 If the **-a** parameter was given, hidden partitions also will be shown.

 If the **-g** parameter was given, the ouput shows each **GRES (gpu, mic etc.)** defined in that partition and (in paranteses) the total number of nodes in that partition containing that GRES:

```
       QUEUE   FREE  TOTAL   FREE  TOTAL RESORC  OTHER MAXJOBTIME   CORES   NODE   GRES
   PARTITION  CORES  CORES  NODES  NODES PENDNG PENDNG NDAY-HR:MN PERNODE MEM-GB (COUNT)
       defq*     56   2436      2     87      0     56    7-00:00      28    128 -
     shortq      56   2604      2     93      0      0    0-01:00      28    128 gpu:2(12)
      longq      72    336      3     14      0      0   21-00:00      24     64 -
       gpuq       0    112      0      4      0      0    7-00:00      28    128 gpu:1(4)
    bigmemq       0    280      0     10     28      0    7-00:00      28    512 -
      b224q      56   2548      2     91      0      0    1-00:00      28    128 -
    core40q       0   1400      0     35   2404      0    7-00:00      40    192 -
        all.    128   4340      5    142      0      0     -           24     64 gpu:1(4),gpu:2(12)
```


 The **-h** parameter prints usage info.


 
 ## Compiling

 Compiling is very simple.

 If your slurm installed at default location, you can compile the spart command as below:

 ```gcc -lslurm spart.c -o spart```

 If not, you should give locations of the headers and libraries:

 ```gcc -lslurm spart.c -o spart -I/location/of/slurm/header/files/ -L/location/of/slurm/library/files/```

 
 Also, there is no need to have administrative rights (being root) to compile and use. If you want to use the spart command as a regular user, you can compile and use at your home directory.


