#!/bin/sh

/bin/awk '
# Function definition
/^[a-zA-Z_0-9]+\(.*\) {/ {
    if (match($0, /[a-zA-Z_0-9]+\(/)) {
        name = substr($0, RSTART, RLENGTH-1)
        match($0, /\(.*\)/);
        params = substr($0, RSTART+1, RLENGTH-2)
        functions[name ",,\"" params "\""] = ""
        current = name
    }
}

# Function call
/[a-zA-Z_0-9]+\(/ {
    if (match($0, /[a-zA-Z_0-9]+\(/)) {
        name = substr($0, RSTART, RLENGTH-1)
        if (name != current)
            functions["\t"current "," name] = ""
    }
}

END {
    sorter = "/bin/sort -bf"
    print "Caller,Target,Params"
    for (f in functions)
        print f | sorter
    close(sorter);
}
' *.c *.h >call-graph.csv && ./spreadsheet call-graph.csv