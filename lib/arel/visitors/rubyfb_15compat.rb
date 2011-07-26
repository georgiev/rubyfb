module Arel
  module Visitors
    class RubyFB_15Compat < Arel::Visitors::ToSql
    	Arel::Visitors::VISITORS['rubyfb'] = Arel::Visitors::RubyFB_15Compat
    private
      def visit_Arel_Nodes_Limit o
      end

      def visit_Arel_Nodes_Offset o
      end
      
      def visit_Arel_Nodes_SelectStatement o
      	super.tap do |s|
      	  if o.limit || o.offset  
      	    s.gsub!(/^\s*select/i, "SELECT #{fb_limit(o.limit)} #{fb_offset(o.offset)} ")
      	  end
      	end
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
