DIRS = src
OUTPUT_PATH = bin

all: prebuild
	set -e; for d in $(DIRS); do $(MAKE) -C $$d ; done

prebuild:
	if [ ! -d "$(OUTPUT_PATH)" ]; then mkdir -p $(OUTPUT_PATH); fi

clean:
	rm -rf $(OUTPUT_PATH)
