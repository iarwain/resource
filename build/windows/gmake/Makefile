# GNU Make solution makefile autogenerated by Premake
# Type "make help" for usage help

ifndef config
  config=debug
endif
export config

PROJECTS := Resource

.PHONY: all clean help $(PROJECTS)

all: $(PROJECTS)

Resource: 
	@echo "==== Building Resource ($(config)) ===="
	@${MAKE} --no-print-directory -C . -f Resource.make

clean:
	@${MAKE} --no-print-directory -C . -f Resource.make clean

help:
	@echo "Usage: make [config=name] [target]"
	@echo ""
	@echo "CONFIGURATIONS:"
	@echo "   debug"
	@echo "   profile"
	@echo "   release"
	@echo ""
	@echo "TARGETS:"
	@echo "   all (default)"
	@echo "   clean"
	@echo "   Resource"
	@echo ""
	@echo "For more information, see http://industriousone.com/premake/quick-start"
