module Rubyfb
  class Connection
    # Creates stored procedure call object
    def prepare_call(procedure_name)
      Rubyfb::ProcedureCall.new(self, procedure_name)
    end
  end
end
