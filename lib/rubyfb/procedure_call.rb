module Rubyfb
  class ProcedureCall
    attr_reader :connection, :procedure_name, :param_names

    def initialize(connection, procedure_name)
      @procedure_name=procedure_name
      @connection=connection
      @param_names = []
      connection.execute_immediate(
        "SELECT RDB$PARAMETER_NAME
        FROM RDB$PROCEDURE_PARAMETERS
        WHERE RDB$PROCEDURE_NAME='#{procedure_name}' and RDB$PARAMETER_TYPE = 0
        ORDER BY RDB$PARAMETER_NUMBER"
      ) do |row|
        row.each do |col, val|
          @param_names << column_key(val)
        end
      end
    end

    # execute stored procedure vith parameter values bound to values
    # passing NULL for all parameters not found in values
    # returns output parameters as a hash - { :output_parameter_name=>value, ... }
    def execute(values={}, transaction=nil)
      result = {}
      execute_statement(values, transaction) do |row|
        row.each do |col, value|
          result[column_key(col)] = value
        end
      end
      return result
    end

    # returns string containing coma separated parameter values
    # corresponding to param_names array, quoted/formated in SQL format
    def sql_value_list(values)
      param_names.collect{|p| quote_value(values[p])}.join(',')
    end

    def quote_value(value)
      # TODO - proper quoting
      value ? "'#{value}'" : "NULL"
    end
  private
    def bind_params(values)
      "(#{sql_value_list(values)})" unless param_names.empty?
    end

    def gen_sql(values)
      "execute procedure #{procedure_name}#{bind_params(values)};"
    end

    def execute_statement(values={}, transaction=nil, &block)
      if transaction
        connection.execute(gen_sql(values), transaction, &block)
      else
        connection.execute_immediate(gen_sql(values), &block)
      end
    end

    def column_key(column_name)
      (column_name =~ /[[:lower:]]/ ? column_name : column_name.downcase).strip.intern
    end
  end
end
