# spart
A user-oriented partition info command for slurm.

Slurm does not contains a command to show user-oriented partition info. I wrote a command. I hope you will find it useful.

## Usage

The spart command accepts a minimal number of options:

```
$ spart --help
usage:

    ./spart {options}

  options:

    -h/--help               display this help

    output formats:

      -t/--text             as a column-aligned textual table
      -p/--parsable         text delimited by vertical bar character
                            (memory values in MB, times in minutes)
      -j/--json             as a JSON dictionary keyed by partition name
                            (memory values in MB, times in minutes)
      -y/--yaml             as a YAML dictionary keyed by partition name
                            (memory values in MB, times in minutes)

```

The command defaults to displaying in the column-aligned textual table format:

```
$ spart
    QUEUE     FREE    TOTAL     FREE    TOTAL RESOURCE    OTHER   MIN   MAX  MAXJOBTIME   CORES    NODE
PARTITION    CORES    CORES    NODES    NODES  PENDING  PENDING NODES NODES   DAY-HR:MN PERNODE  MEM(GB)
 defaultq      448     2436       16       87        0        0     1     -     7-00:00      28     128
   shortq      588     2604       21       93        0        0     1     2     0-01:00      28     128
    longq       96      336        4       14        0        0     1     -    21-00:00      24      64
  bigmemq      252      280        9       10        0        0     1     -     7-00:00      28     512
    b224q      560     2548       20       91        0        0     8    40     1-00:00      28     128
  core40q      480     1400       12       35      900        0     1     -     7-00:00      40     192
      all     1164     4340       37      142        0        0     1     -    NO-LIMIT      24      64
 ```

 The **RESOURCE PENDING** column shows core counts of pending jobs because of the busy resource. The **OTHER PENDING** column shows core counts of pending jobs because of the other reasons such as license or other limits.

 ## Compiling

 If Slurm is installed in standard system locations, no additional flags should be necessary:

 ```gcc -lslurm spart.c -o spart```

 If Slurm is installed elsewhere, the path to the header files and libraries must be specified:

 ```gcc -I/path/to/slurm/install/include -o spart spart.c -L/path/to/slurm/install/lib -lslurm```


## Output formats

The code includes a modular design for outputting the partition summaries.  The original format is presented in the `spart_printer_text` set of functions; it has been altered slightly to size the leading column to meet the maximum partition name length.

The parsable text format (`spart_printer_parsable`) is similar to other Slurm parsable display formats:  values are displayed without whitespace padding with columns delimited by the vertical bar character.  A header row is included.

The JSON and YAML formats (`spart_printer_json` and `spart_printer_yaml`) use key-value dictionaries to organize the information.  The top-level dictionary is keyed by partition name.
