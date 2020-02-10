 # spart
 A user-oriented partition info command for slurm.

 Slurm does not have a command showing partition info in a user-friendly way.
 I wrote a command, I hope you will find it useful. 

## Usage

 **Usage: spart [-m] [-a] [-c] [-g] [-i] [-l] [-h]**

 This program shows **the user specific brief partition info** with core count of available nodes and pending jobs. It hides unnecessary information for users in the output i.e. unusable partitions, undefined limits, unusable nodes etc., but it shows related and usefull information briefly.

 The output of spart without any parameters is as below:

```
 $ spart
     QUEUE STA   FREE  TOTAL   FREE  TOTAL RESORC  OTHER    MIN    MAX MAXJOBTIME  CORES   NODE
 PARTITION TUS  CORES  CORES  NODES  NODES PENDNG PENDNG  NODES  NODES  DAY-HR:MN  /NODE MEM-GB
      defq   *    336   2436     12     87      0      0      1      -    7-00:00     28    126
    shortq        504   2604     18     93      0      0      1      2    0-01:00     28    126
     longq         96    336      4     14      0      0      1      -   21-00:00     24     62
      gpuq        112    112      4      4      0      0      1      -    7-00:00     28    126
   bigmemq        112    280      4     10      0      0      1      -    7-00:00     28    510
     v100q         40     40      1      1      0      0      1      1    1-00:00     40    375
     b224q        448   2548     16     91      0      0      8     40    1-00:00     28    126
   core40q        160   1400      4     35    240      0      1      -    7-00:00     40    190
 
```

 In the **STA-TUS** column, the characters means, the partition is:
```
	*	default partition (default queue),
	.	hidden partition,
	C	closed to both the job submit and run,
	S	closed to the job submit, but the submitted jobs will run,
        r       requires the reservation,
	D	open to the job submit, but the submitted jobs will not run,
	R	open for only root, or closed to root (if you are root),
	A	closed to all of your account(s),
	a	closed to some of your account(s),
	G	closed to all of your group(s),
	g	closed to some of your group(s),
	Q	closed to all of your QOS(s),
	q	closed to some of your QOS(s).
```

 The **RESOURCE PENDING** column shows core counts of pending jobs because of the busy resource.

 The **OTHER PENDING** column shows core counts of pending jobs because of the other reasons such
 as license or other limits.

 If the partition's **QOS NAME, MIN NODES, MAX NODES,** and **MAXJOBTIME** limits are not setted for 
 the all partitions in your cluster, corresponding column(s) will not be shown, except -l
 parameter was given.

 The **CORES /NODE** column shows the core count of the node with lowest core count in the
 partition. But if -l was given, both the lowest and highest core counts will be shown.

 The **NODE MEM-GB** column shows the memory of the lowest memory node in this partition. But if
 -l parameter was given, both the lowest and highest memory will be shown.

 Parameters:

 **-m**	both the lowest and highest values will be shown in the **CORES /NODE**
		and **NODE MEM-GB** columns.

 **-a**	hidden partitions also be shown.

 **-c**	partitions from federated clusters be shown.

 **-g**	the ouput shows each GRES (gpu, mic etc.) defined in that partition
		and (in paranteses) the total number of nodes in that partition
		containing that GRES.

 **-i** the info about the groups, accounts, QOSs, and queues will be shown.

 **-l**	all posible columns will be shown, except the federated clusters column.

 **-h**	shows this usage text.

If you compare the output above with the output with -l parameter (below), unusable and hidden partitions
 were not shown without -l parameter:
```
$ spart -l
     QUEUE STA   FREE  TOTAL   FREE  TOTAL RESORC  OTHER    MIN    MAX MAXJOBTIME    CORES       NODE    QOS   GRES
 PARTITION TUS  CORES  CORES  NODES  NODES PENDNG PENDNG  NODES  NODES  DAY-HR:MN    /NODE     MEM-GB   NAME (COUNT)
      defq   *    532   2436     19     87      0      0      1      -    7-00:00       28    126-510      -  -
    shortq        616   2604     22     93      0      0      1      2    0-01:00       28    126-510      -  gpu:k20m:1(4)
     longq         96    336      4     14      0      0      1      -   21-00:00       24         62      -  -
      gpuq         56    112      2      4      0      0      1      -    7-00:00       28    126-510      -  gpu:k20m:1(4)
   bigmemq         56    280      2     10      0      0      1      -    7-00:00       28        510      -  gpu:k20m:1(1)
     v100q         40     40      1      1      0      0      1      1    1-00:00       40        375      -  gpu:v100:4(1)
      yzmq   A     40     40      1      1      0      0      1      1    7-00:00       40        375      -  gpu:v100:4(1)
     b224q        560   2548     20     91      0      0      8     40    1-00:00       28    126-510      -  gpu:k20m:1(2)
   hbm513q   G    504   2240     18     80      0      0      1     10    0-00:30       28        126      -  -
   core40q        160   1400      4     35    240      0      1      -    7-00:00       40        190      -  -
       all   .    912   4380     31    143      0      0      1      -     -         24-40     62-510      -  gpu:k20m:1(4),gpu:v100:4(1) 

```

The output of the spart with the -i parameter:
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

## The Cluster and Partition Statements

When STATEMET feature is on (the **SPART_SHOW_STATEMENT** macro is defined), the spart looks for
 the statements files. If the spart finds any statement file, it shows the content of the files.
 If the spart can not find a particular staement file, simply ignore it without the notification.
 Therefore, there is no need to recompile to reshow or cancel statements. You can edit or remove
 statement files after compiling. If you want to show again, just write new file(s).

 The spart also can show the statements using different font and background colors. The style of
 the cluster's statement and the partitions' statements can be setted differently.

For example, the output at the below shows the cluster statement file exist:
```
spart
     QUEUE STA   FREE  TOTAL   FREE  TOTAL RESORC  OTHER    MIN    MAX MAXJOBTIME  CORES   NODE
 PARTITION TUS  CORES  CORES  NODES  NODES PENDNG PENDNG  NODES  NODES  DAY-HR:MN  /NODE MEM-GB
      defq   *    308   2436     11     87      0      0      1      -    7-00:00     28    126
    shortq        392   2604     14     93      0      0      1      2    0-01:00     28    126
     longq         96    336      4     14      0      0      1      -   21-00:00     24     62
      gpuq         56    112      2      4      0      0      1      -    7-00:00     28    126
   bigmemq         56    280      2     10      0      0      1      -    7-00:00     28    510
     v100q         40     40      1      1      0      0      1      1    1-00:00     40    375
     b224q        336   2548     12     91      0      0      8     40    1-00:00     28    126
   core40q        160   1400      4     35      0      0      1      -    7-00:00     40    190

   ============================================================================================
   Dear Colleagues,

   Sariyer cluster will be stopped next week, Tuesday, February 14, for scheduled maintenance
   operations. The stop will start at 9:00 a.m. and we expect to bring the cluster back to
   production in the late afternoon. During the course of the maintenance operations, the login
   nodes will not be accessible.

   Best Regards,
   User Support, UHeM
   ============================================================================================
```

When the -i parameter was given, the spart also shows the contents of the partition statement files:
```
spart -i
 Your username: mercan
 Your group(s): hsaat gaussian workshop ansys
 Your account(s): hsaat
 Your qos(s): normal

     QUEUE STA   FREE  TOTAL   FREE  TOTAL RESORC  OTHER    MIN    MAX MAXJOBTIME  CORES   NODE
 PARTITION TUS  CORES  CORES  NODES  NODES PENDNG PENDNG  NODES  NODES  DAY-HR:MN  /NODE MEM-GB

   ============================================================================================

      defq   *    308   2436     11     87      0      0      1      -    7-00:00     28    126

   This is the default queue, i.e., if you don't give a partition name,
   defq is used as the partition name for your job.
   --------------------------------------------------------------------------------------------

    shortq        392   2604     14     93      0      0      1      2    0-01:00     28    126
   --------------------------------------------------------------------------------------------
     longq         96    336      4     14      0      0      1      -   21-00:00     24     62

   The nodes in the longq queue are older than those in other queues. Therefore,
   it's better not to use this queue, unless you really need the long time limit.
   --------------------------------------------------------------------------------------------

      gpuq         56    112      2      4      0      0      1      -    7-00:00     28    126

   Each of the nodes in the gpuq queue contain one, very old Nvidia K20m GPU.
   --------------------------------------------------------------------------------------------

   bigmemq         56    280      2     10      0      0      1      -    7-00:00     28    510
   --------------------------------------------------------------------------------------------
     v100q         40     40      1      1      0      0      1      1    1-00:00     40    375
   --------------------------------------------------------------------------------------------
     b224q        336   2548     12     91      0      0      8     40    1-00:00     28    126
   --------------------------------------------------------------------------------------------
   core40q        160   1400      4     35      0      0      1      -    7-00:00     40    190
   --------------------------------------------------------------------------------------------

   ============================================================================================
   Dear Colleagues,

   Sariyer cluster will be stopped next week, Tuesday, February 14, for scheduled maintenance
   operations. The stop will start at 9:00 a.m. and we expect to bring the cluster back to
   production in the late afternoon. During the course of the maintenance operations, the login
   nodes will not be accessible.

   Best Regards,
   User Support, UHeM
   ============================================================================================
```

## Requirements


 Some features of the spart requires Slurm 18.08 or newer. With older versions, the spart works with
 reduced feature set i.e. without showing the federated clusters column and user-spesific output.

 
## Compiling

 Compiling is very simple.

 If your slurm installed at default location, you can compile the spart command as below:

 ```gcc -lslurm -lslurmdb spart.c -o spart```
 
 At SLURM 19.05, you should compile without **-libslurmdb**:
 
 ```gcc -lslurm spart.c -o spart```

 If the slurm is not installed at default location, you should add locations of the headers and libraries:

 ```gcc -lslurm -lslurmdb spart.c -o spart -I/location/of/slurm/header/files/ -L/location/of/slurm/library/files/```

 
 Also, there is no need to have administrative rights (being root) to compile and use. If you want to use
 the spart command as a regular user, you can compile and use at your home directory.

## Debug

 If you notice a bug of the spart, please report using the github site.

 It is not possible to test spart on the every slurm configuration and cluster. Thanks.


