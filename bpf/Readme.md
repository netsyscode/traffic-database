# BPF Code
## Description
* The tagging code example is shown as `tag.c`
* Including two tags:
	* the length of each packet
	* the TCP flags of TCP packet
	
## Compile

```
clang -O2 -emit-llvm -S tag.c
llc -march=bpf -filetype=obj tag.ll
```

* It will be compiled to `tag.o`, which can be hooked at hook point