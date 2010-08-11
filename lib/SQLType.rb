#-------------------------------------------------------------------------------
# SQLType.rb
#-------------------------------------------------------------------------------
# Copyright © Peter Wood, 2005
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with the
# License. You may obtain a copy of the License at
#
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
# the specificlanguage governing rights and  limitations under the License.
#
# The Original Code is the FireRuby extension for the Ruby language.
#
# The Initial Developer of the Original Code is Peter Wood. All Rights
# Reserved.

module FireRuby
   # This class is used to represent SQL table column tables.
   class SQLType
      # A definition for a base SQL type.
      BIGINT                           = :BIGINT

      # A definition for a base SQL type.
      BLOB                             = :BLOB

      # A definition for a base SQL type.
      CHAR                             = :CHAR

      # A definition for a base SQL type.
      DATE                             = :DATE

      # A definition for a base SQL type.
      DECIMAL                          = :DECIMAL

      # A definition for a base SQL type.
      DOUBLE                           = :DOUBLE

      # A definition for a base SQL type.
      FLOAT                            = :FLOAT

      # A definition for a base SQL type.
      INTEGER                          = :INTEGER

      # A definition for a base SQL type.
      NUMERIC                          = :NUMERIC

      # A definition for a base SQL type.
      SMALLINT                         = :SMALLINT

      # A definition for a base SQL type.
      TIME                             = :TIME

      # A definition for a base SQL type.
      TIMESTAMP                        = :TIMESTAMP

      # A definition for a base SQL type.
      VARCHAR                          = :VARCHAR

      # Attribute accessor.
      attr_reader :type, :length, :precision, :scale, :subtype


      # This is the constructor for the SQLType class.
      #
      # ==== Parameters
      # type::       The base type for the SQLType object. Must be one of the
      #              base types defined within the class.
      # length::     The length setting for the type. Defaults to nil.
      # precision::  The precision setting for the type. Defaults to nil.
      # scale::      The scale setting for the type. Defaults to nil.
      # subtype::    The SQL sub-type setting. Defaults to nil.
      def initialize(type, length=nil, precision=nil, scale=nil, subtype=nil)
         @type      = type
         @length    = length
         @precision = precision
         @scale     = scale
         @subtype   = subtype
      end


      # This class method fetches the type details for a named table. The
      # method returns a hash that links column names to SQLType objects.
      #
      # ==== Parameters
      # table::       A string containing the name of the table.
      # connection::  A reference to the connection to be used to determine
      #               the type information.
      #
      # ==== Exception
      # FireRubyException::  Generated if an invalid table name is specified
      #                      or an SQL error occurs.
      def SQLType.for_table(table, connection)
         # Check for naughty table names.
         if /\s+/ =~ table
            raise FireRubyException.new("'#{table}' is not a valid table name.")
         end

         types     = {}
         begin
            sql = "SELECT RF.RDB$FIELD_NAME, F.RDB$FIELD_TYPE, "\
                  "F.RDB$FIELD_LENGTH, F.RDB$FIELD_PRECISION, "\
                  "F.RDB$FIELD_SCALE * -1, F.RDB$FIELD_SUB_TYPE "\
                  "FROM RDB$RELATION_FIELDS RF, RDB$FIELDS F "\
                  "WHERE RF.RDB$RELATION_NAME = UPPER('#{table}') "\
                  "AND RF.RDB$FIELD_SOURCE = F.RDB$FIELD_NAME"

            connection.start_transaction do |tx|
               tx.execute(sql) do |row|
                  sql_type = SQLType.to_base_type(row[1], row[5])
                  type     = nil
                  case sql_type
                     when BLOB
                        type = SQLType.new(sql_type, nil, nil, nil, row[5])

                     when CHAR, VARCHAR
                        type = SQLType.new(sql_type, row[2])

                     when DECIMAL, NUMERIC
                        type = SQLType.new(sql_type, nil, row[3], row[4])

                     else
                        type = SQLType.new(sql_type)
                  end
                  types[row[0].strip] = type
               end

            end
         end
         types
      end


      # This method overloads the equivalence test operator for the SQLType
      # class.
      #
      # ==== Parameters
      # object::  A reference to the object to be compared with.
      def ==(object)
         result = false
         if object.instance_of?(SQLType)
            result = (@type      == object.type &&
                      @length    == object.length &&
                      @precision == object.precision &&
                      @scale     == object.scale &&
                      @subtype   == object.subtype)
         end
         result
      end


      # This method generates a textual description for a SQLType object.
      def to_s
         if @type == SQLType::DECIMAL or @type == SQLType::NUMERIC
            "#{@type.id2name}(#{@precision},#{@scale})"
         elsif @type == SQLType::BLOB
            "#{@type.id2name} SUB TYPE #{@subtype}"
         elsif @type == SQLType::CHAR or @type == SQLType::VARCHAR
            "#{@type.id2name}(#{@length})"
         else
            @type.id2name
         end
      end


      # This class method converts a Firebird internal type to a SQLType base
      # type.
      #
      # ==== Parameters
      # type::     A reference to the Firebird field type value.
      # subtype::  A reference to the Firebird field subtype value.
      def SQLType.to_base_type(type, subtype)
         case type
            when 16  # BIGINT, DECIMAL, NUMERIC
               case subtype
                  when 1 then SQLType::NUMERIC
                  when 2 then SQLType::DECIMAL
                  else SQLType::BIGINT
               end

            when 261 # BLOB
               SQLType::BLOB

            when 14  # CHAR
               SQLType::CHAR

            when 12  # DATE
               SQLType::DATE

            when 27  # DOUBLE
               SQLType::DOUBLE

            when 10  # FLOAT
               SQLType::FLOAT

            when 8   # INTEGER, DECIMAL, NUMERIC
               case subtype
                  when 1 then SQLType::NUMERIC
                  when 2 then SQLType::DECIMAL
                  else SQLType::INTEGER
               end

            when 7   # SMALLINT, DECIMAL, NUMERIC
               case subtype
                  when 1 then SQLType::NUMERIC
                  when 2 then SQLType::DECIMAL
                  else SQLType::SMALLINT
               end

            when 13  # TIME
               SQLType::TIME

            when 35  # TIMESTAMP
               SQLType::TIMESTAMP

            when 37  # VARCHAR
               SQLType::VARCHAR
         end
      end
   end # End of the SQLType class.
end # End of the FireRuby module.