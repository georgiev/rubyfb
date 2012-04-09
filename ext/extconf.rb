#!/usr/bin/env ruby

ENV['ARCHFLAGS']='-arch '+`arch`.strip if RUBY_PLATFORM.include?("darwin")

require 'mkmf'

# Add the framework link for Mac OS X.
if RUBY_PLATFORM.include?("darwin")
   $LDFLAGS = $LDFLAGS + " -framework Firebird"
   $CFLAGS  = $CFLAGS + " -DOS_UNIX"
   firebird_include="/Library/Frameworks/Firebird.framework/Headers"
   firebird_lib="/Library/Frameworks/Firebird.framework/Libraries"
elsif RUBY_PLATFORM.include?("win32")
   $LDFLAGS = $LDFLAGS + " fbclient_ms.lib"
   $CFLAGS  = "-MT #{$CFLAGS}".gsub!(/-MD\s*/, '') + " -DOS_WIN32"
   dir_config("win32")
   dir_config("winsdk")
   dir_config("dotnet")
   firebird_include="../mswin32fb"
   firebird_lib="../mswin32fb"
elsif RUBY_PLATFORM.include?("mingw32")
   $LIBS = $LIBS + " ../mswin32fb/fbclient_mingw.def -lfbclient_mingw"
   $DLDFLAGS = "--enable-stdcall-fixup,"+$DLDFLAGS
   firebird_include="../mswin32fb"
   firebird_lib="../mswin32fb"
elsif RUBY_PLATFORM.include?("linux")
   $LDFLAGS = $LDFLAGS + " -lfbclient -lpthread"
   $CFLAGS  = $CFLAGS + " -DOS_UNIX"
elsif  RUBY_PLATFORM.include?("freebsd")
   $LDFLAGS = $LDFLAGS + " -lfbclient -lpthread"
   $CFLAGS  = $CFLAGS + " -DOS_UNIX"
   firebird_include = "/usr/local/include"
   firebird_lib="/usr/local/lib"
   
end

# Make sure the firebird stuff is included.
dir_config("firebird", firebird_include, firebird_lib)

# Generate the Makefile.
create_makefile("rubyfb_lib")
