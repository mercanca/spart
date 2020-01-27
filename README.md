 # spart
 A user-oriented partition info command for slurm.

 Slurm does not have a command showing partition info in a user-friendly way.
 I wrote a command, I hope you will find it useful. 

## Usage

 **Usage: spart [-m] [-a] [-c] [-g] [-i] [-l] [-h]**

 This program shows **the user specific brief partition info** with core count of available nodes and pending jobs.

 The output of spart without any parameters is as below:

```
 $ spart
     QUEUE STA   FREE  TOTAL   FREE  TOTAL RESORC  OTHER    MIN    MAX MAXJOBTIME  CORES   NODE
 PARTITION TUS  CORES  CORES  NODES  NODES PENDNG PENDNG  NODES  NODES  DAY-HR:MN  /NODE MEM-GB
      defq   *   1036   2436     37     87      0    840      1      -    7-00:00     28    128
    shortq       1064   2604     38     93      0      0      1      2    0-01:00     28    128
     longq         48    336      2     14      0      0      1      -   21-00:00     24     64
      gpuq         28    112      1      4      0      0      1      -    7-00:00     28    128
   bigmemq         28    280      1     10      0      0      1      -    7-00:00     28    512
     b224q       1064   2548     38     91      0      0      8     40    1-00:00     28    128
   core40q         40   1400      1     35   1456      0      1      -    7-00:00     40    192
```

 In the **STA-TUS** column, the characters means, the partition is:
```
	*	default partition (default queue),
	.	hidden partition,
	C	closed to both the job submit and run,
	S	closed to the job submit, but the submitted jobs will run,
	D	open to the job submit, but the submitted jobs will not run,
	r	requires the reservation,
	R	open for only root, or closed to root (if you are root),
	A	closed to your account(s),
	G	closed to your group(s).
```

 The **RESOURCE PENDING** column shows core counts of pending jobs because of the busy resource.

 The **OTHER PENDING** column shows core counts of pending jobs because of the other reasons such
 as license or other limits.

 If **MIN NODES, MAX NODES,** and **MAXJOBTIME** limits are not setted for the all partitions in your
 cluster, corresponding column(s) will not be shown.

 The **CORES /NODE** column shows the core count of the node with lowest core count in the partition.

 The **NODE MEM-GB** column shows the memory of the lowest memory node in this partition.

 Parameters:

 **-m**	both the lowest and highest values will be shown in the **CORES /NODE**
		and **NODE MEM-GB** columns.

 **-a**	hidden partitions also be shown.

 **-c**	partitions from federated clusters be shown.

 **-g**	the ouput shows each GRES (gpu, mic etc.) defined in that partition
		and (in paranteses) the total number of nodes in that partition
		containing that GRES.

 **-i** the groups and accounts info will be shown.

 **-l**	all posible columns will be shown, except the federated clusters column.

 **-h**	shows this usage text.

```
$ spart -l
     QUEUE STA   FREE  TOTAL   FREE  TOTAL RESORC  OTHER    MIN    MAX MAXJOBTIME    CORES       NODE   GRES
 PARTITION TUS  CORES  CORES  NODES  NODES PENDNG PENDNG  NODES  NODES  DAY-HR:MN    /NODE     MEM-GB (COUNT)
      defq   *   1036   2436     37     87      0    840      1      -    7-00:00       28    128-512 -
    shortq       1064   2604     38     93      0      0      1      2    0-01:00       28    128-512 gpu:1(4)
     longq         48    336      2     14      0      0      1      -   21-00:00       24         64 -
      gpuq         28    112      1      4      0      0      1      -    7-00:00       28    128-512 gpu:1(4)
   bigmemq         28    280      1     10      0      0      1      -    7-00:00       28        512 gpu:1(1)
     b224q       1064   2548     38     91      0      0      8     40    1-00:00       28    128-512 gpu:1(2)
   core40q         40   1400      1     35   1456      0      1      -    7-00:00       40        192 -
       all  .A   1152   4340     41    142      0      0      1      -     -         24-40     64-512 gpu:1(4)
```

```
$ spart -i
 Your username: marfea
 Your group(s): eoss1 d0001
 Your account(s): d0001 eoss1
 Your qos(s): normal

     QUEUE STA   FREE  TOTAL   FREE  TOTAL RESORC  OTHER    MIN    MAX MAXJOBTIME  CORES   NODE
 PARTITION TUS  CORES  CORES  NODES  NODES PENDNG PENDNG  NODES  NODES  DAY-HR:MN  /NODE MEM-GB
      defq   *    868   2436     31     87   1024      0      1      -    7-00:00     28    126
    shortq        924   2604     33     93      0      0      1      2    0-01:00     28    126
     longq        264    336     11     14      0      0      1      -   21-00:00     24     62
      gpuq         56    112      2      4      0      0      1      -    7-00:00     28    126
   bigmemq         56    280      2     10      0      0      1      -    7-00:00     28    510
     b224q        896   2548     32     91      0      0      8     40    1-00:00     28    126
   core40q         40   1400      1     35    444      0      1      -    7-00:00     40    190
```

## Requirements


 Some features of the spart requires Slurm 18.08 or newer. With older versions, the spart works with reduced feature set such as without showing the federated clusters column and user-spesific output.

 
## Compiling

 Compiling is very simple.

 If your slurm installed at default location, you can compile the spart command as below:

 ```gcc -lslurm -lslurmdb spart.c -o spart```
 
 At SLURM 19.05, you should compile without **-libslurmdb**:
 
 ```gcc -lslurm spart.c -o spart```

 If it is not installed at default location, you should add locations of the headers and libraries:

 ```gcc -lslurm -lslurmdb spart.c -o spart -I/location/of/slurm/header/files/ -L/location/of/slurm/library/files/```

 
 Also, there is no need to have administrative rights (being root) to compile and use. If you want to use the spart command as a regular user, you can compile and use at your home directory.


