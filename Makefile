DISABLE=-z execstack -no-pie -fcf-protection=none -z norelro -Wno-format-security

canaries=$(foreach i,0 1 2,canary_$i)
nocanaries=no-canary_0

all: $(canaries) $(nocanaries)

canary_%: canary.c
	gcc -g $(DISABLE) -fstack-protector -DCANARY_$* -o canary_$* canary.c

no-canary_%: canary.c
	gcc -g $(DISABLE) -fno-stack-protector -DCANARY_$* -o no-canary_$* canary.c

.PHONY: clean
clean:
	rm -f canary_* no-canary_*

+%:
	@echo $*=$($*)
