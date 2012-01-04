module Arel
  module Visitors
    class RubyFB < Arel::Visitors::ToSql
    private
      def visit_Arel_Nodes_Limit o
      end

      def visit_Arel_Nodes_Offset o
      end
    
      def visit_Arel_Nodes_SelectStatement o
      	o.limit || o.offset ? "SELECT #{fb_limit(o.limit)} #{fb_offset(o.offset)} * FROM (#{super})" : super
      end

      def fb_limit limit
      	"first #{limit.expr}" if limit
      end

      def fb_offset offset
      	"skip #{offset.expr}" if offset
      end
    end
  end
end
