CC ?= cc
PKGS = gtk4 json-glib-1.0 epoxy
CPPFLAGS += $(patsubst -I%,-isystem%,$(shell pkg-config --cflags $(PKGS)))
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic
LDLIBS += $(shell pkg-config --libs $(PKGS)) -lm
SOURCES = main.c model.c assets.c render.c ui.c
OBJECTS = $(SOURCES:%.c=build/%.o)

.PHONY: build run test clean
build: build/stage_editor
build/stage_editor: $(OBJECTS)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@
build/%.o: %.c stage_editor.h | build/
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@
build/:
	mkdir -p $@
run: build
	./build/stage_editor
build/tests: tests.c model.c assets.c stage_editor.h | build/
	$(CC) $(CPPFLAGS) $(CFLAGS) tests.c model.c assets.c $(LDLIBS) -o $@
test: build/tests
	./build/tests
clean:
	$(RM) -r build
-include $(OBJECTS:.o=.d)
