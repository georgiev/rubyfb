# Author: Ken Kunz <kennethkunz@gmail.com>
require 'active_record/connection_adapters/abstract_adapter'
require 'active_support/core_ext/kernel/requires'

if defined?(Arel) then
  if Rubyfb::Options.fb15_compat
    require 'arel/visitors/rubyfb_15compat'
  else
    require 'arel/visitors/rubyfb'
  end
end

module Rubyfb # :nodoc: all
  NON_EXISTENT_DOMAIN_ERROR = "335544569"
  class Database
    def self.db_string_for(config)
      unless config.has_key?(:database)
        raise ArgumentError, "No database specified. Missing argument: database."
      end
      host_string = config.values_at(:host, :service, :port).compact.first(2).join("/") if config[:host]
      [host_string, config[:database]].compact.join(":")
    end

    def self.new_from_config(config)
      db = new db_string_for(config)
      db.character_set = config[:charset]
      return db
    end
  end

  class ProcedureCall
    class SQLParser < ActiveRecord::Base
      def self.bind_params(sql_array)
        sanitize_sql_array(sql_array)
      end
    end

    def sql_value_list(values)
      SQLParser.bind_params([param_names.collect{|p| '?'}.join(',')] + param_names.collect{|p| values[p]})
    end
  end
end

module ActiveRecord
  class Base
    def self.rubyfb_connection(config) # :nodoc:
      require_library_or_gem 'rubyfb'
      config.symbolize_keys!
      db = Rubyfb::Database.new_from_config(config)
      connection_params = config.values_at(:username, :password)
      if config[:sql_role_name]
        connection_params << {Rubyfb::Connection::SQL_ROLE_NAME=>config[:sql_role_name]}
      end
      connection = db.connect(*connection_params)
      ConnectionAdapters::RubyfbAdapter.new(connection, logger, connection_params)
    end

    after_save :write_blobs
    def write_blobs #:nodoc:
      if connection.is_a?(ConnectionAdapters::RubyfbAdapter)
        connection.write_blobs(self.class.table_name, self.class, attributes)
      end
    end

    private :write_blobs
  end

  module ConnectionAdapters
    class FirebirdColumn < Column # :nodoc:
      VARCHAR_MAX_LENGTH = 32_765

      def initialize(connection, name, domain, type, sub_type, length, precision, scale, default_source, null_flag)
        @firebird_type = Rubyfb::SQLType.to_base_type(type, sub_type).to_s

        super(name.downcase, nil, @firebird_type, !null_flag)
        
        @limit = decide_limit(length)
        @domain, @sub_type, @precision, @scale = domain, sub_type, precision, (scale.nil? ? 0 : scale.abs)
        @type = simplified_type(@firebird_type)
        @default = parse_default(default_source) if default_source
        @default = type_cast(decide_default(connection)) if @default
      end

      def self.value_to_boolean(value)
        %W(#{RubyfbAdapter.boolean_domain[:true]} true t 1).include? value.to_s.downcase
      end

      private
        def parse_default(default_source)
          default_source =~ /^\s*DEFAULT\s+(.*)\s*$/i
          return $1 unless $1.upcase == "NULL"
        end

        def decide_default(connection)
          if @default =~ /^'?(\d*\.?\d+)'?$/ or
             @default =~ /^'(.*)'$/ && [:text, :string, :binary, :boolean].include?(type)
            $1
          else
            firebird_cast_default(connection)
          end
        end

        # Submits a _CAST_ query to the database, casting the default value to the specified SQL type.
        # This enables Firebird to provide an actual value when context variables are used as column
        # defaults (such as CURRENT_TIMESTAMP).
        def firebird_cast_default(connection)
          sql = "SELECT CAST(#{@default} AS #{column_def}) FROM RDB$DATABASE"
          connection.select_rows(sql).first[0]
        end

        def decide_limit(length)
          if text? or number?
            length
          end
        end

        def column_def
          case @firebird_type
            when 'BLOB'               then "VARCHAR(#{VARCHAR_MAX_LENGTH})"
            when 'CHAR', 'VARCHAR'    then "#{@firebird_type}(#{@limit})"
            when 'NUMERIC', 'DECIMAL' then "#{@firebird_type}(#{@precision},#{@scale.abs})"
            when 'DOUBLE'             then "DOUBLE PRECISION"
            else @firebird_type
          end
        end

        def simplified_type(field_type)
          case field_type
            when /timestamp/i
              :datetime
            when /decimal|numeric|number/i
              @scale == 0 ? :integer : :decimal
            when /blob/i
              @subtype == 1 ? :text : :binary
            else
              if @domain =~ RubyfbAdapter.boolean_domain[:domain_pattern] || name =~ RubyfbAdapter.boolean_domain[:name_pattern]
                :boolean
              else
                super
              end
          end
        end
    end

    # == Usage Notes
    #
    # === Sequence (Generator) Names
    # The Firebird adapter supports the same approach adopted for the Oracle
    # adapter. See ActiveRecord::Base#set_sequence_name for more details.
    #
    # Note that in general there is no need to create a <tt>BEFORE INSERT</tt>
    # trigger corresponding to a Firebird sequence generator when using
    # ActiveRecord. In other words, you don't have to try to make Firebird
    # simulate an <tt>AUTO_INCREMENT</tt> or +IDENTITY+ column. When saving a
    # new record, ActiveRecord pre-fetches the next sequence value for the table
    # and explicitly includes it in the +INSERT+ statement. (Pre-fetching the
    # next primary key value is the only reliable method for the Firebird
    # adapter to report back the +id+ after a successful insert.)
    #
    # === BOOLEAN Domain
    # Firebird 1.5 does not provide a native +BOOLEAN+ type. But you can easily
    # define a +BOOLEAN+ _domain_ for this purpose, e.g.:
    #
    #  CREATE DOMAIN D_BOOLEAN AS SMALLINT CHECK (VALUE IN (0, 1) OR VALUE IS NULL);
    #
    # When the Firebird adapter encounters a column that is based on a domain
    # that includes "BOOLEAN" in the domain name, it will attempt to treat
    # the column as a +BOOLEAN+.
    #
    # By default, the Firebird adapter will assume that the BOOLEAN domain is
    # defined as above.  This can be modified if needed.  For example, if you
    # have a legacy schema with the following +BOOLEAN+ domain defined:
    #
    #  CREATE DOMAIN BOOLEAN AS CHAR(1) CHECK (VALUE IN ('T', 'F'));
    #
    # ...you can add the following lines to your <tt>environment.rb</tt> file:
    #
    #  ActiveRecord::ConnectionAdapters::RubyfbAdapter.boolean_domain[:true] = 'T'
    #  ActiveRecord::ConnectionAdapters::RubyfbAdapter.boolean_domain[:false] = 'F'
    #
    #  Boolean columns are detected by name and domain name patterns configurable by:
    #
    #  ActiveRecord::ConnectionAdapters::RubyfbAdapter.boolean_domain[:domain_pattern], default = /boolean/i
    #  ActiveRecord::ConnectionAdapters::RubyfbAdapter.boolean_domain[:name_pattern], default = /^is_/i
    #
    # === BLOB Elements
    # The Firebird adapter currently provides only limited support for +BLOB+
    # columns. You cannot currently retrieve a +BLOB+ as an IO stream.
    # When selecting a +BLOB+, the entire element is converted into a String.
    # +BLOB+ handling is supported by writing an empty +BLOB+ to the database on
    # insert/update and then executing a second query to save the +BLOB+. 
    #
    # === Column Name Case Semantics
    # Firebird and ActiveRecord have somewhat conflicting case semantics for
    # column names.
    #
    # [*Firebird*]
    #   The standard practice is to use unquoted column names, which can be
    #   thought of as case-insensitive. (In fact, Firebird converts them to
    #   uppercase.) Quoted column names (not typically used) are case-sensitive.
    # [*ActiveRecord*]
    #   Attribute accessors corresponding to column names are case-sensitive.
    #   The defaults for primary key and inheritance columns are lowercase, and
    #   in general, people use lowercase attribute names.
    #
    # In order to map between the differing semantics in a way that conforms
    # to common usage for both Firebird and ActiveRecord, uppercase column names
    # in Firebird are converted to lowercase attribute names in ActiveRecord,
    # and vice-versa. Mixed-case column names retain their case in both
    # directions. Lowercase (quoted) Firebird column names are not supported.
    # This is similar to the solutions adopted by other adapters.
    #
    # In general, the best approach is to use unqouted (case-insensitive) column
    # names in your Firebird DDL (or if you must quote, use uppercase column
    # names). These will correspond to lowercase attributes in ActiveRecord.
    #
    # For example, a Firebird table based on the following DDL:
    #
    #  CREATE TABLE products (
    #    id BIGINT NOT NULL PRIMARY KEY,
    #    "TYPE" VARCHAR(50),
    #    name VARCHAR(255) );
    #
    # ...will correspond to an ActiveRecord model class called +Product+ with
    # the following attributes: +id+, +type+, +name+.
    #
    # ==== Quoting <tt>"TYPE"</tt> and other Firebird reserved words:
    # In ActiveRecord, the default inheritance column name is +type+. The word
    # _type_ is a Firebird reserved word, so it must be quoted in any Firebird
    # SQL statements. Because of the case mapping described above, you should
    # always reference this column using quoted-uppercase syntax
    # (<tt>"TYPE"</tt>) within Firebird DDL or other SQL statements (as in the
    # example above). This holds true for any other Firebird reserved words used
    # as column names as well.
    #
    # === Migrations
    # The Firebird Adapter now supports Migrations.
    #
    # ==== Create/Drop Table and Sequence Generators
    # Creating or dropping a table will automatically create/drop a
    # correpsonding sequence generator, using the default naming convension.
    # You can specify a different name using the <tt>:sequence</tt> option; no
    # generator is created if <tt>:sequence</tt> is set to +false+.
    #
    # ==== Rename Table
    # The Firebird #rename_table Migration should be used with caution.
    # Firebird 1.5 lacks built-in support for this feature, so it is
    # implemented by making a copy of the original table (including column
    # definitions, indexes and data records), and then dropping the original
    # table. Constraints and Triggers are _not_ properly copied, so avoid
    # this method if your original table includes constraints (other than
    # the primary key) or triggers. (Consider manually copying your table
    # or using a view instead.)
    #
    # == Connection Options
    # The following options are supported by the Firebird adapter. None of the
    # options have default values.
    #
    # <tt>:database</tt>::
    #   <i>Required option.</i> Specifies one of: (i) a Firebird database alias;
    #   (ii) the full path of a database file; _or_ (iii) a full Firebird
    #   connection string. <i>Do not specify <tt>:host</tt>, <tt>:service</tt>
    #   or <tt>:port</tt> as separate options when using a full connection
    #   string.</i>
    # <tt>:host</tt>::
    #   Set to <tt>"remote.host.name"</tt> for remote database connections.
    #   May be omitted for local connections if a full database path is
    #   specified for <tt>:database</tt>. Some platforms require a value of
    #   <tt>"localhost"</tt> for local connections when using a Firebird
    #   database _alias_.
    # <tt>:service</tt>::
    #   Specifies a service name for the connection. Only used if <tt>:host</tt>
    #   is provided. Required when connecting to a non-standard service.
    # <tt>:port</tt>::
    #   Specifies the connection port. Only used if <tt>:host</tt> is provided
    #   and <tt>:service</tt> is not. Required when connecting to a non-standard
    #   port and <tt>:service</tt> is not defined.
    # <tt>:username</tt>::
    #   Specifies the database user. May be omitted or set to +nil+ (together
    #   with <tt>:password</tt>) to use the underlying operating system user
    #   credentials on supported platforms.
    # <tt>:password</tt>::
    #   Specifies the database password. Must be provided if <tt>:username</tt>
    #   is explicitly specified; should be omitted if OS user credentials are
    #   are being used.
    # <tt>:charset</tt>::
    #   Specifies the character set to be used by the connection. Refer to
    #   Firebird documentation for valid options.
    # <tt>:sql_role_name</tt>::
    #   Specifies the SQL role name used by the connection.
    class RubyfbAdapter < AbstractAdapter
      TEMP_COLUMN_NAME = 'AR$TEMP_COLUMN'

      @@boolean_domain = { :name => "d_boolean", :type => "smallint", :true => 1, :false => 0, :domain_pattern=>/boolean/i, :name_pattern=>/^is_/i }
      cattr_accessor :boolean_domain

      def initialize(connection, logger, connection_params = nil)
        super(connection, logger)
        @connection_params = connection_params
      end

      def adapter_name # :nodoc:
        'Rubyfb'
      end

      def supports_migrations? # :nodoc:
        true
      end

      def native_database_types # :nodoc:
        {
          :primary_key => "BIGINT NOT NULL PRIMARY KEY",
          :string      => { :name => "varchar", :limit => 255 },
          :text        => { :name => "blob sub_type text" },
          :integer     => { :name => "bigint" },
          :decimal     => { :name => "decimal" },
          :numeric     => { :name => "numeric" },
          :float       => { :name => "float" },
          :datetime    => { :name => "timestamp" },
          :timestamp   => { :name => "timestamp" },
          :time        => { :name => "time" },
          :date        => { :name => "date" },
          :binary      => { :name => "blob sub_type 0" },
          :boolean     => boolean_domain
        }
      end

      # Returns true for Firebird adapter (since Firebird requires primary key
      # values to be pre-fetched before insert). See also #next_sequence_value.
      def prefetch_primary_key?(table_name = nil)
        true
      end

      def default_sequence_name(table_name, primary_key = nil) # :nodoc:
        "#{table_name}_seq"
      end


      # QUOTING ==================================================

      # We use quoting in order to implement BLOB handling. In order to
      # do this we quote a BLOB to an empty string which will force Firebird
      # to create an empty BLOB in the db for us. Quoting is used in some
      # other places besides insert/update like for column defaults. That is
      # why we are checking caller to see where we're coming from. This isn't
      # perfect but It works.
      def quote(value, column = nil) # :nodoc:
        if [Time, DateTime].include?(value.class)
          "CAST('#{value.strftime("%Y-%m-%d %H:%M:%S")}' AS TIMESTAMP)"
        elsif value && column && [:text, :binary].include?(column.type) && caller.to_s !~ /add_column_options!/i 
          "''"
        else
          super
        end
      end

      def quote_string(string) # :nodoc:
        string.gsub(/'/, "''")
      end

      def quote_column_name(column_name) # :nodoc:
        %Q("#{ar_to_fb_case(column_name.to_s)}")
      end

      def quoted_true # :nodoc:
        quote(boolean_domain[:true])
      end

      def quoted_false # :nodoc:
        quote(boolean_domain[:false])
      end


      # CONNECTION MANAGEMENT ====================================

      def active? # :nodoc:
	return false if @connection.closed?
	begin
		execute('select first 1 cast(1 as smallint) from rdb$database')
		true
	rescue
		false
	end
      end

      def disconnect! # :nodoc:
        @connection.close rescue nil
      end

      def reconnect! # :nodoc:
        disconnect!
        @connection = @connection.database.connect(*@connection_params)
      end


      # DATABASE STATEMENTS ======================================

      def select_rows(sql, name = nil)
        select_raw(sql, name).last
      end

      def execute(sql, name = nil, &block) # :nodoc:
        exec_result = execute_statement(sql, name, &block)
        if exec_result.instance_of?(Rubyfb::ResultSet)
          exec_result.close
          exec_result = nil
        end
        return exec_result
      end

      def begin_db_transaction() # :nodoc:
        @transaction = @connection.start_transaction
      end

      def commit_db_transaction() # :nodoc:
        @transaction.commit
      ensure
        @transaction = nil
      end

      def rollback_db_transaction() # :nodoc:
        @transaction.rollback
      ensure
        @transaction = nil
      end

      def add_limit_offset!(sql, options) # :nodoc:
        if options[:limit]
          limit_string = "FIRST #{options[:limit]}"
          limit_string << " SKIP #{options[:offset]}" if options[:offset]
          sql.sub!(/\A(\s*SELECT\s)/i, '\&' + limit_string + ' ')
        end
      end

      # Returns the next sequence value from a sequence generator. Not generally
      # called directly; used by ActiveRecord to get the next primary key value
      # when inserting a new database record (see #prefetch_primary_key?).
      def next_sequence_value(sequence_name)
        Rubyfb::Generator.new(sequence_name, @connection).next(1)
      end

      # Inserts the given fixture into the table. Overridden to properly handle blobs.
      def insert_fixture(fixture, table_name)
        super

        klass = fixture.class_name.constantize rescue nil
        if klass.respond_to?(:ancestors) && klass.ancestors.include?(ActiveRecord::Base)
          write_blobs(table_name, klass, fixture)
        end
      end

      # Writes BLOB values from attributes, as indicated by the BLOB columns of klass.      
      def write_blobs(table_name, klass, attributes)
        id = quote(attributes[klass.primary_key])
        klass.columns.select { |col| col.sql_type =~ /BLOB$/i }.each do |col|
          value = attributes[col.name]
          value = value.to_yaml if col.text? && klass.serialized_attributes[col.name]
          value = value.read if value.respond_to?(:read)
          next if value.nil?  || (value == '')
          s = Rubyfb::Statement.new(@connection, @transaction, "UPDATE #{table_name} set #{col.name} = ? WHERE #{klass.primary_key} = #{id}", 3)       
          s.execute_for([value.to_s])
          s.close
        end
      end


      # SCHEMA STATEMENTS ========================================

      def current_database # :nodoc:
        file = @connection.database.file.split(':').last
        File.basename(file, '.*')
      end

      def recreate_database! # :nodoc:
        sql = "SELECT rdb$character_set_name FROM rdb$database"
        charset = select_rows(sql).first[0].rstrip
        disconnect!
        @connection.database.drop(*@connection_params)
        Rubyfb::Database.create(@connection.database.file,
          @connection_params[0], @connection_params[1], 4096, charset)
      end

      def tables(name = nil) # :nodoc:
        sql = "SELECT rdb$relation_name FROM rdb$relations WHERE rdb$system_flag = 0"
        select_rows(sql, name).collect { |row| row[0].rstrip.downcase }
      end

      def table_exists?(table_name)
        super(table_name.to_s.downcase)
      end

      def primary_key(table_name)
        if pk_row = index_metadata(table_name, true).to_a.first
      		pk_row[2].rstrip.downcase
 	    	end
      end

      def indexes(table_name, name = nil) # :nodoc:
        index_metadata(table_name, false, name).inject([]) do |indexes, row|
          if indexes.empty? or indexes.last.name != row[0]
            indexes << IndexDefinition.new(table_name, row[0].rstrip.downcase, row[1] == 1, [])
          end
          indexes.last.columns << row[2].rstrip.downcase
          indexes
        end
      end

      def columns(table_name, name = nil) # :nodoc:
        sql = <<-end_sql
          SELECT r.rdb$field_name, r.rdb$field_source, f.rdb$field_type, f.rdb$field_sub_type,
                 f.rdb$field_length, f.rdb$field_precision, f.rdb$field_scale,
                 COALESCE(r.rdb$default_source, f.rdb$default_source) rdb$default_source,
                 COALESCE(r.rdb$null_flag, f.rdb$null_flag) rdb$null_flag
          FROM rdb$relation_fields r
          JOIN rdb$fields f ON r.rdb$field_source = f.rdb$field_name
          WHERE r.rdb$relation_name = '#{table_name.to_s.upcase}'
          ORDER BY r.rdb$field_position
        end_sql

        select_rows(sql, name).collect do |row|
          field_values = row.collect do |value|
            case value
              when String then value.rstrip
              else value
            end
          end
          FirebirdColumn.new(self, *field_values)
        end
      end

      def create_table(name, options = {}) # :nodoc:
        begin
          super
        rescue StatementInvalid
          raise unless non_existent_domain_error?
          create_boolean_domain
          super
        end
        unless options[:id] == false or options[:sequence] == false
          sequence_name = options[:sequence] || default_sequence_name(name)
          create_sequence(sequence_name)
        end
      end

      def drop_table(name, options = {}) # :nodoc:
        super(name)
        unless options[:sequence] == false
          sequence_name = options[:sequence] || default_sequence_name(name)
          drop_sequence(sequence_name) if sequence_exists?(sequence_name)
        end
      end

      def add_column(table_name, column_name, type, options = {}) # :nodoc:
        super
      rescue StatementInvalid
        raise unless non_existent_domain_error?
        create_boolean_domain
        super
      end

      def change_column(table_name, column_name, type, options = {}) # :nodoc:
        change_column_type(table_name, column_name, type, options)
        change_column_position(table_name, column_name, options[:position]) if options.include?(:position)
        change_column_default(table_name, column_name, options[:default]) if options_include_default?(options)
        change_column_null(table_name, column_name, options[:null], options[:default]) if options.key?(:null)
      end

      def change_column_default(table_name, column_name, default) # :nodoc:
        table_name = table_name.to_s.upcase
        sql = <<-end_sql
          UPDATE rdb$relation_fields f1
          SET f1.rdb$default_source =
                (SELECT f2.rdb$default_source FROM rdb$relation_fields f2
                 WHERE f2.rdb$relation_name = '#{table_name}'
                 AND f2.rdb$field_name = '#{TEMP_COLUMN_NAME}'),
              f1.rdb$default_value =
                (SELECT f2.rdb$default_value FROM rdb$relation_fields f2
                 WHERE f2.rdb$relation_name = '#{table_name}'
                 AND f2.rdb$field_name = '#{TEMP_COLUMN_NAME}')
          WHERE f1.rdb$relation_name = '#{table_name}'
          AND f1.rdb$field_name = '#{ar_to_fb_case(column_name.to_s)}'
        end_sql
        transaction do
          add_column(table_name, TEMP_COLUMN_NAME, :string, :default => default)
          execute_statement(sql)
          remove_column(table_name, TEMP_COLUMN_NAME)
        end
      end

      def change_column_null(table_name, column_name, null, default = nil)
        table_name = table_name.to_s.upcase
        column_name = column_name.to_s.upcase

        unless null || default.nil?
          execute_statement("UPDATE #{quote_table_name(table_name)} SET #{quote_column_name(column_name)}=#{quote(default)} WHERE #{quote_column_name(column_name)} IS NULL")
        end
        execute_statement("UPDATE RDB$RELATION_FIELDS SET RDB$NULL_FLAG = #{null ? 'null' : '1'} WHERE (RDB$FIELD_NAME = '#{column_name}') and (RDB$RELATION_NAME = '#{table_name}')")
      end

      def rename_column(table_name, column_name, new_column_name) # :nodoc:
        execute_statement("ALTER TABLE #{quote_table_name(table_name)} ALTER COLUMN #{quote_column_name(column_name)} TO #{quote_column_name(new_column_name)}")
      end

      def remove_index(table_name, options) #:nodoc:
        execute_statement("DROP INDEX #{quote_column_name(index_name(table_name, options))}")
      end

      def rename_table(name, new_name) # :nodoc:
        if table_has_constraints_or_dependencies?(name)
          raise ActiveRecordError,
            "Table #{name} includes constraints or dependencies that are not supported by " <<
            "the Firebird rename_table migration. Try explicitly removing the constraints/" <<
            "dependencies first, or manually renaming the table."
        end

        transaction do
          copy_table(name, new_name)
          copy_table_indexes(name, new_name)
        end
        begin
          copy_table_data(name, new_name)
          copy_sequence_value(name, new_name)
        rescue
          drop_table(new_name)
          raise
        end
        drop_table(name)
      end

      def dump_schema_information # :nodoc:
        super << ";\n"
      end

      def type_to_sql(type, limit = nil, precision = nil, scale = nil) # :nodoc:
        case type
          when :integer then integer_sql_type(limit)
          when :float   then float_sql_type(limit)
          when :string  then super(type, limit, precision, scale)
          else super(type, limit, precision, scale)
        end
      end

      def execute_procedure(procedure_name, values={})
        @connection.prepare_call(procedure_name).execute(values, @transaction)
      end

      private
        def execute_statement(sql, name = nil, &block) # :nodoc:
	  @fbe = nil
          log(sql, name) do
	    begin
              if @transaction
                @connection.execute(sql, @transaction, &block)
              else
                @connection.execute_immediate(sql, &block)
              end
	    rescue Exception => e
	      @fbe = e
              raise e
	    end
          end
	rescue Exception => se
	  def se.nested=value
	    @nested=value
	  end
	  def se.nested
	    @nested
	  end
	  se.nested = @fbe
	  raise se
        end

        def integer_sql_type(limit)
          case limit
            when (1..2) then 'smallint'
            when (3..4) then 'integer'
            else 'bigint'
          end
        end

        def float_sql_type(limit)
          limit.to_i <= 4 ? 'float' : 'double precision'
        end

        def select(sql, name = nil)
          fields, rows = select_raw(sql, name)
          result = []
          for row in rows
            row_hash = {}
            fields.each_with_index do |f, i|
              row_hash[f] = row[i]
            end
            result << row_hash
          end
          result
        end

        def select_raw(sql, name = nil)
          fields = []
          rows = []
          execute_statement(sql, name) do |row|
            array_row = []
            row.each do |column, value|
              fields << fb_to_ar_case(column) if row.number == 1

              if Rubyfb::Blob === value
                temp = value.to_s
                value.close
                value = temp
              end
              array_row << value
            end
            rows << array_row
          end
          return fields, rows 
        end

        def index_metadata(table_name, pk, name = nil)
          sql = <<-end_sql
            SELECT i.rdb$index_name, i.rdb$unique_flag, s.rdb$field_name
            FROM rdb$indices i
            JOIN rdb$index_segments s ON i.rdb$index_name = s.rdb$index_name
            LEFT JOIN rdb$relation_constraints c ON i.rdb$index_name = c.rdb$index_name
            WHERE i.rdb$relation_name = '#{table_name.to_s.upcase}'
          end_sql
          if pk
            sql << "AND c.rdb$constraint_type = 'PRIMARY KEY'\n"
          else
            sql << "AND (c.rdb$constraint_type IS NULL OR c.rdb$constraint_type != 'PRIMARY KEY')\n"
          end
          sql << "ORDER BY i.rdb$index_name, s.rdb$field_position\n"
          execute_statement(sql, name)
        end

        def change_column_type(table_name, column_name, type, options = {})
          sql = "ALTER TABLE #{quote_table_name(table_name)} ALTER COLUMN #{quote_column_name(column_name)} TYPE #{type_to_sql(type, options[:limit])}"
          execute_statement(sql)
        rescue StatementInvalid
          raise unless non_existent_domain_error?
          create_boolean_domain
          execute_statement(sql)
        end

        def change_column_position(table_name, column_name, position)
          execute_statement("ALTER TABLE #{quote_table_name(table_name)} ALTER COLUMN #{quote_column_name(column_name)} POSITION #{position}")
        end

        def copy_table(from, to)
          table_opts = {}
          if pk = primary_key(from)
            table_opts[:primary_key] = pk
          else
            table_opts[:id] = false
          end
          create_table(to, table_opts) do |table|
            from_columns = columns(from).reject { |col| col.name == table_opts[:primary_key] }
            from_columns.each do |column|
              col_opts = [:limit, :default, :null].inject({}) { |opts, opt| opts.merge(opt => column.send(opt)) }
              table.column column.name, column.type, col_opts
            end
          end
        end

        def copy_table_indexes(from, to)
          indexes(from).each do |index|
            unless index.name[from.to_s]
              raise ActiveRecordError,
                "Cannot rename index #{index.name}, because the index name does not include " <<
                "the original table name (#{from}). Try explicitly removing the index on the " <<
                "original table and re-adding it on the new (renamed) table."
            end
            options = {}
            options[:name] = index.name.gsub(from.to_s, to.to_s)
            options[:unique] = index.unique
            add_index(to, index.columns, options)
          end
        end

        def copy_table_data(from, to)
          execute_statement("INSERT INTO #{to} SELECT * FROM #{from}", "Copy #{from} data to #{to}")
        end

        def copy_sequence_value(from, to)
          sequence_value = Rubyfb::Generator.new(default_sequence_name(from), @connection).last
          execute_statement("SET GENERATOR #{default_sequence_name(to)} TO #{sequence_value}")
        end

        def sequence_exists?(sequence_name)
          Rubyfb::Generator.exists?(sequence_name, @connection)
        end

        def create_sequence(sequence_name)
          Rubyfb::Generator.create(sequence_name.to_s, @connection)
        end

        def drop_sequence(sequence_name)
          Rubyfb::Generator.new(sequence_name.to_s, @connection).drop
        end

        def create_boolean_domain
          sql = <<-end_sql
            CREATE DOMAIN #{boolean_domain[:name]} AS #{boolean_domain[:type]}
            CHECK (VALUE IN (#{quoted_true}, #{quoted_false}) OR VALUE IS NULL)
          end_sql
          execute_statement(sql) rescue nil
        end

        def table_has_constraints_or_dependencies?(table_name)
          table_name = table_name.to_s.upcase
          sql = <<-end_sql
            SELECT 1 FROM rdb$relation_constraints
            WHERE rdb$relation_name = '#{table_name}'
            AND rdb$constraint_type IN ('UNIQUE', 'FOREIGN KEY', 'CHECK')
            UNION
            SELECT 1 FROM rdb$dependencies
            WHERE rdb$depended_on_name = '#{table_name}'
            AND rdb$depended_on_type = 0
          end_sql
          !select(sql).empty?
        end

        def non_existent_domain_error?
          $!.message.include? Rubyfb::NON_EXISTENT_DOMAIN_ERROR
        end

        # Maps uppercase Firebird column names to lowercase for ActiveRecord;
        # mixed-case columns retain their original case.
        def fb_to_ar_case(column_name)
          column_name =~ /[[:lower:]]/ ? column_name : column_name.downcase
        end

        # Maps lowercase ActiveRecord column names to uppercase for Fierbird;
        # mixed-case columns retain their original case.
        def ar_to_fb_case(column_name)
          column_name =~ /[[:upper:]]/ ? column_name : column_name.upcase
        end
    end
  end
end
