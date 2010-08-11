#!/usr/bin/env ruby

ENV['ARCHFLAGS']='-arch '+`arch`.strip if PLATFORM.include?("darwin")

require 'mkmf'

# Add the framework link for Mac OS X.
if PLATFORM.include?("darwin")
   $LDFLAGS = $LDFLAGS + " -framework Firebird"
   $CFLAGS  = $CFLAGS + " -DOS_UNIX"
   firebird_include="/Library/Frameworks/Firebird.framework/Headers"
   firebird_lib="/Library/Frameworks/Firebird.framework/Libraries"
elsif PLATFORM.include?("win32")
   $LDFLAGS = $LDFLAGS + " fbclient_ms.lib"
   $CFLAGS  = $CFLAGS + " -DOS_WIN32"
   dir_config("win32")
   dir_config("winsdk")
   dir_config("dotnet")
elsif PLATFORM.include?("linux")
   $LDFLAGS = $LDFLAGS + " -lfbclient -lpthread"
   $CFLAGS  = $CFLAGS + " -DOS_UNIX"
end

# Make sure the firebird stuff is included.
dir_config("firebird", firebird_include, firebird_lib)

# Generate the Makefile.
create_makefile("fireruby")
