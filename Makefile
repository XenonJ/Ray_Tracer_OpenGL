ASSIGN     = demo
BREWPATH   = $(shell brew --prefix)
CXX        = $(shell fltk-config --cxx)
CXXFLAGS   = $(shell fltk-config --cxxflags) -Iinclude -I$(BREWPATH)/include
LDFLAGS    = $(shell fltk-config --ldflags --use-gl --use-images) -L$(BREWPATH)/lib
POSTBUILD  = fltk-config --post # build .app for osx. (does nothing on pc)

SRCDIR     = src
HEADERDIR  = include
OBJDIR     = obj

SRCS       = $(shell find $(SRCDIR) -name "*.cpp")
OBJS       = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCS))

$(ASSIGN): $(OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@
	$(POSTBUILD) $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(ASSIGN) $(ASSIGN).app $(OBJDIR) *~ *.dSYM