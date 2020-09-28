 # spart
 A user-oriented partition info command for slurm. It gives a brief view of the cluster.

 Slurm does not have a command showing partition info in a user-friendly way. 
 I wrote a command, I hope you will find it useful. 

## Usage

 **Usage: spart [-m] [-a] [-c] [-g] [-i] [-t] [-f] [-s] [-J] [-l] [-v] [-h]**

 This program shows **the user specific partition info** with core count of available nodes and pending jobs. It hides unnecessary information for users in the output i.e. unusable partitions, undefined limits, unusable nodes etc., but it shows related and usefull information such as how many pending jobs waiting for the resourses or for the other reasons.

 The output of spart without any parameters is as below:

```
 $ spart
     QUEUE STA   FREE  TOTAL RESORC  OTHER   FREE  TOTAL |YOUR PEND PEND YOUR |   MIN   MAX    MAXIMUM  CORES   NODE
 PARTITION TUS  CORES  CORES PENDNG PENDNG  NODES  NODES | RUN  RES OTHR TOTL | NODES NODES   JOB-TIME  /NODE MEM-GB
      defq   *     84   2436    140     28      3     87 |   3    5    0    8 |     1     -     7 days     28    126
    shortq         84   2604      0      0      3     93 |   0    0    0    0 |     1     2     1 hour     28    126
     longq        120    336      0    120      5     14 |   1    0    0    1 |     1     -    21 days     24     62
      gpuq          0    112      0      0      0      4 |   1    0    0    1 |     1     -     7 days     28    126
   bigmemq   C     56    280      0      0      2     10 |   0    0    0    0 |     1     -     7 days     28    510
     v100q          0     40      0      0      0      1 |   0    0    0    0 |     1     1     1 days     40    375
     b224q         84   2548      0    840      3     91 |   3    0    3    6 |     8    40     7 days     28    126
   core40q   g      0   1400    560    400      0     35 |   0    0    0    0 |     1     -     7 days     40    190
 
```
The spart command output varies according to cluster configuration to help to the user. You can see very different output of the spart for a different cluster at below. Notice the columns added. Without the spart command, it is very difficult to see the configuration details of the slurm cluster:

```
$ spart
WARNING: The Slurm settings have info restrictions!
	the spart can not show other users' waiting jobs info!

        QUEUE STA   FREE  TOTAL RESORC  OTHER   FREE  TOTAL ||   MAX DEFMEM MAXMEM    MAXIMUM  CORES   NODE    QOS
    PARTITION TUS  CORES  CORES PENDNG PENDNG  NODES  NODES || NODES GB/CPU G/NODE   JOB-TIME  /NODE MEM-GB   NAME
     defaultq   *    295   2880      0      0      0    120 ||     -      4    124    15 days     24    128      -
       single        110    144      0      0      3      6 ||     -      9    252    15 days     24    256      -
          smp        184    224      0      0      0      1 ||     -     17   4121     8 days    224   4128      -
        short        736   9172      0      0      0    278 ||     -      8    252     4 hour     24    256      -
          mid        736   9172      0      0      0    278 ||     -      8    252     8 days     24    256      -
         long        736   9172      0      0      0    278 ||     -      8    252    15 days     24    256      -
        debug       1633  14532      0      0      8    461 ||     4      8    252    15 mins     24    128  debug

                  YOUR PEND PEND YOUR   MIN    DEFAULT
                   RUN  RES OTHR TOTL NODES   JOB-TIME
   COMMON VALUES:    0    0    0    0     1     2 mins

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
	a	closed to some of your accounts,
	G	closed to all of your group(s),
	g	closed to some of your groups,
	Q	closed to all of your QOS(s),
	q	closed to some of your QOSs.
```

The **RESOURCE PENDING** column shows core counts of pending jobs because of the busy resource.

The **OTHER PENDING** column shows core counts of pending jobs because of the other reasons such
 as license or other limits.

The **YOUR-RUN, PEND-RES, PEND-OTHR**, and **YOUR-TOTL** columns shows the counts of the running,
 resource pending, other pending, and total job count of the current user, respectively.
 If these four columns are have same values, These same values of that four columns will be
 shown at COMMON VALUES as four single values.

The **MIN NODE** and **MAX NODE** columns show the permitted minimum and maximum node counts of the
 jobs which can be submited to the partition.

The **MAXCPU/NODE** column shows the permitted maximum core counts of of the single node in
 the partition.


The **DEFMEM GB/CPU** and **DEFMEM GB/NODE** columns show default maximum memory as GB which a job
 can use for a cpu or a node, respectively.

The **MAXMEM GB/CPU** and **MAXMEM GB/NODE** columns show maximum memory as GB which requestable by
 a job for a cpu or a node, respectively.

The **DEFAULT JOB-TIME** column shows the default time limit of the job which submited to the
 partition without a time limit. If the DEFAULT JOB-TIME limits are not setted, or setted
 same value with MAXIMUM JOB-TIME for all partitions in your cluster, DEFAULT JOB-TIME
 column will not be shown, except -l parameter was given.

The **MAXIMUM JOB-TIME** column shows the maximum time limit of the job which submited to the
 partition. If the user give a time limit further than MAXIMUM JOB-TIME limit of the
 partition, the job will be rejected by the slurm.


The **CORES /NODE** column shows the core count of the node with lowest core count in the
 partition. But if -l was given, both the lowest and highest core counts will be shown.

The **NODE MEM-GB** column shows the memory of the lowest memory node in this partition. But if
 -l parameter was given, both the lowest and highest memory will be shown.

The **QOS NAME** column shows the default qos limits the job which submited to the partition.
 If the QOS NAME of the partition are not setted for all partitions in your cluster, QOS NAME
 column will not be shown, execpt -l parameter was given.

The **GRES (COUNT)** column shows the generic resources of the nodes in the partition, and (in
 paranteses) the total number of nodes in that partition containing that GRES. The GRES (COUNT)
 column will not be shown, execpt -l or -g parameter was given.

If the partition's **QOS NAME, MIN NODES, MAX NODES, MAXCPU/NODE, DEFMEM GB/CPU|NODE,
 MAXMEM GB/CPU|NODE, DEFAULT JOB-TIME**, and **MAXIMUM JOB-TIME** limits are not setted for the
 all partitions in your cluster, corresponding column(s) will not be shown, except -l
 parameter was given.

If the values of a column are same, this column will not be shown at partitions block.
 These same values of that column will be shown at **COMMON VALUES** as a single value.

Parameters:

 **-m**	both the lowest and highest values will be shown in the **CORES /NODE**
		and **NODE MEM-GB** columns.

 **-a**	hidden partitions also be shown.

 **-c**	partitions from federated clusters be shown.

 **-g**	the ouput shows each GRES (gpu, mic etc.) defined in that partition
		and (in paranteses) the total number of nodes in that partition
		containing that GRES.

 **-i** the info about the groups, accounts, QOSs, and queues will be shown.
 
 **-t**	the time info will be shown at DAY-HR:MN format, instead of verbal format.

 **-s** the simple output. spart don't show slurm config columns.

 **-J**	the output does not shown the info about the user's jobs.

 **-f** the ouput shows each FEATURES defined in that partition and (in paranteses)
		the total number of nodes in that partition containing that FEATURES.

 **-l**	all posible columns will be shown, except the federated clusters column.

 **-v** shows info about STATUS LABELS.

 **-h**	shows this usage text.

If you compare the output above with the output with -l parameter (below), unusable and hidden partitions
 were not shown without -l parameter:
```
$ spart -l
     QUEUE STA   FREE  TOTAL RESORC  OTHER   FREE  TOTAL ||  MIN    MAX    DEFAULT    MAXIMUM    CORES       NODE  GRES
 PARTITION TUS  CORES  CORES PENDNG PENDNG  NODES  NODES || NODES NODES   JOB-TIME   JOB-TIME    /NODE     MEM-GB (NODE-COUNT)
      defq   *      0   2436    532      0      0     87 ||    1      -     7 days     7 days       28    126-510 -
    shortq          0   2604      0      0      0     93 ||    1      2     1 hour     1 hour       28    126-510 gpu:k20m:1(4)
     longq         72    336      0      0      3     14 ||    1      -    21 days    21 days       24         62 -
      gpuq          0    112      0      0      0      4 ||    1      -     7 days     7 days       28    126-510 gpu:k20m:1(4)
   bigmemq          0    280      0      0      0     10 ||    1      -     7 days     7 days       28        510 gpu:k20m:1(1)
     v100q          0     40      0      0      0      1 ||    1      1     1 days     1 days       40        375 gpu:v100:4(1)
      yzmq   A      0     40     40      0      0      1 ||    1      1     7 days     7 days       40        375 gpu:v100:4(1)
     b224q          0   2548    364      0      0     91 ||    8     40     7 days     7 days       28    126-510 gpu:k20m:1(2)
   hbm513q   G      0   2240      0      0      0     80 ||    1     10    30 mins    30 mins       28        126 -
   core40q   C      0   1400      0      0      0     35 ||    1      -     7 days     7 days       40        190 -
   coronaq   g      0   1400      0   1400      0     35 ||    1      -     7 days     7 days       40        190 -
       all   .     72   4380      0      0      3    143 ||    1      -     1 days     -         24-40     62-510 gpu:k20m:1(4),gpu:v100:4(1)

                  YOUR PEND PEND YOUR MAXCPU DEFMEM MAXMEM    QOS
                   RUN  RES OTHR TOTL  /NODE G/NODE G/NODE   NAME
   COMMON VALUES:    0    0    0    0      -      -      -      -

```

The output of the spart with the -i parameter:
```
$ spart -i
 Your username: marfea
 Your group(s): eoss1 d0001
 Your account(s): d0001 eoss1
 Your qos(s): normal

     QUEUE STA   FREE  TOTAL RESORC  OTHER   FREE  TOTAL ||   MIN    MAX    MAXIMUM  CORES   NODE
 PARTITION TUS  CORES  CORES PENDNG PENDNG  NODES  NODES || NODES  NODES   JOB-TIME  /NODE MEM-GB
      defq   *      0   2436    532      0      0     87 ||     1      -     7 days     28    126
    shortq          0   2604      0      0      0     93 ||     1      2     1 hour     28    126
     longq         72    336      0      0      3     14 ||     1      -    21 days     24     62
      gpuq          0    112      0      0      0      4 ||     1      -     7 days     28    126
   bigmemq          0    280      0      0      0     10 ||     1      -     7 days     28    510
     v100q          0     40      0      0      0      1 ||     1      1     1 days     40    375
     b224q          0   2548    364      0      0     91 ||     8     40     7 days     28    126
   core40q   C      0   1400      0      0      0     35 ||     1      -     7 days     40    190
   coronaq   g      0   1400      0   1400      0     35 ||     1      -     7 days     40    190

                  YOUR PEND PEND YOUR
                   RUN  RES OTHR TOTL
   COMMON VALUES:    0    0    0    0

```

## The Cluster and Partition Statements

When STATEMENT feature is on, the spart looks for the statements files. To open the STATEMENT 
 feature, the **SPART_SHOW_STATEMENT** macro must be defined that can be achieved when you 
 uncomment line 24 in spart.h file. If the spart finds any statement file, it shows the content 
 of the files. If the spart can not find a particular staement file, simply ignore it without
 the notification. Therefore, there is no need to recompile to reshow or cancel statements.
 You can edit or remove statement files after compiling. If you want to show again, just write
 new file(s).

The spart also can show the statements using different font sytles and background colors. The 
 style of the cluster's statement and the partitions' statements can be setted differently.

For example, the output at the below shows the cluster statement file exist:
```
$ spart
     QUEUE STA   FREE  TOTAL RESORC  OTHER   FREE  TOTAL   MIN    MAX MAXJOBTIME  CORES   NODE
 PARTITION TUS  CORES  CORES PENDNG PENDNG  NODES  NODES NODES  NODES  DAY-HR:MN  /NODE MEM-GB
      defq   *    280   2436    400      0     10     87     1      -    7-00:00     28    126
    shortq        420   2604      0      0     15     93     1      2    0-01:00     28    126
     longq        336    336      0      0     14     14     1      -   21-00:00     24     62
      gpuq         84    112      0      0      3      4     1      -    7-00:00     28    126
   bigmemq        140    280      0      0      5     10     1      -    7-00:00     28    510
     v100q         40     40      0      0      1      1     1      1    1-00:00     40    375
     b224q        364   2548      0    280     13     91     8     40    1-00:00     28    126
   core40q        400   1400      0      0     10     35     1      -    7-00:00     40    190

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
$ spart -i
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

The spart requires a running slurm which compiled with mysql/mariadb.

Some features of the spart requires Slurm 18.08 or newer. With older versions, the spart works with
 reduced feature set i.e. without showing the federated clusters column and user-spesific output.

Also, with Slurm 20.02.0 and 20.02.1, the slurm have a bug about to connect the slurm database via
 slurm api. Because of that, the spart works with reduced feature set at this version, too.

The spart requires, the slurm configured to give permision for reading other users job info,
 node info, and other information.

 
## Compiling

Compiling is very simple.

If your slurm installed at default location, you can compile the spart command as below:

 ```gcc -lslurm spart.c -o spart```

Don't add optimization flags (-O2 etc.).
 
At before SLURM 19.05, you should compile with **-lslurmdb**:
 
 ```gcc -lslurm -lslurmdb spart.c -o spart```

If the slurm is not installed at default location, you should add locations of the headers and libraries:

 ```gcc -lslurm -lslurmdb spart.c -o spart -I/location/of/slurm/header/files/ -L/location/of/slurm/library/files/```

After compiling, you can copy the spart file to the default slurm exe directory which is /usr/bin. Alternatively, you can copy spart file to any directory and you should set PATH environment variable. The default slurm man directory is /usr/share/man/man1/. You can copy the man file (spart.1.gz) to this directory, or you can set MANPATH variable. Don't forget to set reading perpisions of the spart and spart.1.gz files for all users.

 
Also, there is no need to have administrative rights (being root) to compile and use. If you want to use
 the spart command as a regular user, you can compile and use at your home directory.

## Debug

If you notice a bug of the spart, please report using the issues page of the github site. Thanks.


