class Numeric
    def seconds
        self
    end
    def second
        self
    end
end

Known_resources = []
class Resource_type
    attr_reader :states
    def initialize(&block)
        @states = {}
        instance_eval &block
    end
    def state(tag,*values)
        values = [false,true] if values.empty?
        @states[tag] = values
    end
    def new(name,args={})
        A_resource.new(self,name,args)
    end
end

class A_resource
    attr_reader :type,:should,:name,:transitions,:side_efects
    def initialize(type,name,args={})
        @type = type
        @name = name
        Known_resources << self
        @state = {}
        puts "#{name} transitions:"
        @transitions = {}
        (args.delete(:transitions)||{}).each { |pvv,x|
            p,*vv = *pvv
            v1s,v2s = *vv.collect { |v|
                case v
                  when '*';   type.states[p]
                  when Array; v
                  else        [v]
                  end
            }
            v1s.each { |v1| v2s.each { |v2|
                k = [p,v1,v2]
                @transitions[k] ||= {}
                @transitions[k][:requires] ||= []
                @transitions[k][:requires] |= x[:requires] || []
            }}
        }
        @transitions.each { |k,v|
            p [k.join(':'),v.collect { |x| x.join(':') }]
        }
        @side_effects = []
        (args.delete(:side_effects)||[]).each { |t|

        }
        @should = args
        @type.states.each { |prop,vals| @state[prop] = vals.first }
    end
    def [](prop)
        @state[prop]
    end
    def to_s
        name
    end
end

A_hand = Resource_type.new do
    state :holding,  :nothing,:milk,:bowl,:spoon,:cereal
end

A_fridge = Resource_type.new do
    state :door, :closed,:open
end

A_cupboard = Resource_type.new do
    state :door, :closed,:open
end

A_milk_bottle = Resource_type.new do
    state :location,  :in_fridge,:mid_air,:on_counter
end

A_cereal_box = Resource_type.new do
    state :location,  :in_cupboard,:mid_air,:on_counter
end

A_spoon = Resource_type.new do
    state :location,  :in_cupboard,:on_counter
end

A_bowl = Resource_type.new do
    state :location,  :in_cupboard,:on_counter
    state :contains_cereal
    state :contains_milk
end

def cost(a,b,delta_t)
    (state_cost(a) + state_cost(b))*delta_t/2
end

def state_cost(a)
    0.5+rand/10000.0+
      ((a[$sd.index([Milk,:location])] == :fridge) ? 0.0 : 2.0)+
      ((a[$sd.index([Fridge,:door])] == :closed) ? 0.0 : 2.5) +
      ((a[$sd.index([Bowl,:contains_milk])] and not a[$sd.index([Bowl,:contains_cereal])]) ? 0.5 : 0.0)
end

def neighbors(s)
    result = []
    $sd.each_with_index { |rp,i|
        r,p = *rp
        vs = $ssoi[rp]
        #puts "#{r}.#{p} can be #{vs.inspect}, in s it is #{s[i]}"
        (vs-[s[i]]).each { |v|
            trans = r.transitions[[p,s[i],v]] || {}
            reqs = trans[:requires] || []
            if reqs.all? { |rd,pd,vd| s[$sd.index([rd,pd]) || fail("#{rd}.#{pd} not in $sd")] == vd } 
                s2 = s.dup
                s2[i] = v
                result << [s2,[r,p,v],trans[:takes] || 1.second]
            end
        }
    }
    result
end

def make_it_so
    puts "Identifying substates of interest"
    srand
    substates_of_interest = Hash.new { |h,k| h[k] = [] }
    Known_resources.each { |r|
         r.transitions.each { |rpvv,t|
             (t[:requires] || []).each { |rpv|
                 rpv[0] = eval(rpv[0]) if rpv[0].is_a? String
             }
         }
         r.should.each { |p,v|
             substates_of_interest[[r,p]] |= [r[p],v] unless r[p] == v
             #puts "#{r}.#{p} is #{r[p]} instead of #{v}" unless r[p] == v
         }
    }
    puts "unaugmented:",substates_of_interest.collect { |rp,vs| "#{rp.join('.')} in #{vs.inspect}" }.join('; ')
    substates_of_interest.each { |rp,vs| r,p = *rp; vs << r.type.states[p]; vs.flatten!; vs.uniq! }
    puts "augmented:",substates_of_interest.collect { |rp,vs| "#{rp.join('.')} in #{vs.inspect}" }.join('; ')
    done = false
    while !done
        done = true
        substates_of_interest.collect { |rp,vs|
            r,p = *rp
            vs.each { |v1| vs.each { |v2| if v1 != v2
                #puts "May need to take #{p} of #{r} from #{v1} to #{v2}"
                ((r.transitions[[p,v1,v2]]||{})[:requires] || []).each { |rd,pd,vd|
                    substates_of_interest[[rd,pd]] |= [rd[pd]]
                    unless substates_of_interest[[rd,pd]].include? vd
                        done = false
                        #puts "Adding #{rd}.#{pd} : #{vd}"
                        substates_of_interest[[rd,pd]] << vd
                    end
                }
            end } }
        }
    end
    $ssoi = substates_of_interest
    $sd = state_dimensions = substates_of_interest.keys
    p state_dimensions.collect { |r,p| "#{r}.#{p}" }
    #puts "#{substates_of_interest.inject(1) { |n,rpvs| n*rpvs.last.length }} possible states of interest"
    current_state = state_dimensions.collect {|r,p| r[p] }
    goal = state_dimensions.collect { |r,p| r.should[p] }
    p current_state
    p goal
    puts "Computing reachable states"
    reachable_states = {current_state => [0,nil,nil]}
    front = neighbors(current_state).collect { |n,op,delta_t| [n,cost(current_state,n,delta_t),current_state,op] }
    op = x = nil
    while (x,c,b,op = front.shift) and x do
        #p front.size
        next if reachable_states.include? x
        #puts op.join(':')
        reachable_states[x] = [c,b,op]
        break if (0...(state_dimensions.length)).all? { |i| !goal[i] or x[i] == goal[i] }
        front += neighbors(x).
          reject  { |n,op,delta_t| reachable_states.include? n }.
          collect { |n,op,delta_t| [n,c+cost(x,n,delta_t),x,op] }
        front = front.sort_by { |x| x[1] }
    end
    puts "#{reachable_states.length} reachable states:"
    #reachable_states.to_a.sort_by {|s,cbop| cbop[0]}.each { |s,cbop|
    #    c,b,op = *cbop
    #    #p [c,(op||[]).join(':'),{b=>s}]
    #}
    puts "Finding path"
    ans = []
    while x
        ans << op << [c,x]
        c,x,op = *reachable_states[x]
    end
    ans.shift
    puts "Answer (#{ans.length} steps):"
    #p ans.reverse
    ans.reverse.each { |s|
        if s.length == 3
            puts "%s.%s <-- %s" % s
        else
            next
            puts ((("    (%s) "+"  \t%s.%s:%s"*s[1].length).gsub(/((  \t%s.%s:%s){3})/,"\\1\n    ")  % ([s[0]]+state_dimensions.zip(s[1]).flatten)).gsub(/the /,''))
        end
    }
end

#-------------------------------

Hand = A_hand.new('your hand',
    :transitions => ({
        [:holding,:spoon, :nothing] => {:requires => [["Spoon", :location,:on_counter]]},
        [:holding,:bowl,  :nothing] => {:requires => [["Bowl",  :location,:on_counter]]},
        [:holding,:milk,  :nothing] => {:requires => [["Milk",  :location,:on_counter]]},
        [:holding,:cereal,:nothing] => {:requires => [["Cereal",:location,:on_counter]]}
    }.merge(
        (os = [:spoon,:bowl,:milk,:cereal]).inject({}) { |h,o|
            (os-[o]).inject(h) { |h,o2| h[[:holding,o,o2]] = {:requires => [["Hand",:holding,o2]]}; h }
        }
    ))
)
Fridge   = A_fridge.new('the fridge', 
    :door => :closed,
    :side_effects => [
        [:door,:open,0,5],
    ],
    :transitions => {
        [:door,:open,:closed] => {:takes => 1.second},
        [:door,:closed,:open] => {:takes => 1.second,:requires => [[Hand,:holding,:nothing]]}
    }
)
Cupboard = A_cupboard.new('the cupboard', 
    :door => :closed,
    :transitions => {
        [:door,:open,:closed] => {:takes => 1.second},
        [:door,:closed,:open] => {:takes => 1.second,:requires => [[Hand,:holding,:nothing]]}
    }
)
Milk = A_milk_bottle.new('the milk',
    :location => :in_fridge,
    :side_effects => [
        [:location,[:mid_air,:in_cupboard,:on_counter],0,1]
    ],
    :transitions => {
        [:location,'*',:mid_air]   => {:requires => [[Hand,:holding,:milk]]},
        [:location,'*',:in_fridge] => {:requires => [[Fridge,:door,:open],[Hand,:holding,:nothing]]},
        [:location,:in_fridge,'*'] => {:requires => [[Fridge,:door,:open]]}
    }
)
Cereal = A_cereal_box.new('the cereal',
    :location => :in_cupboard,
    :transitions => {
        [:location,'*',:mid_air]     => {:requires => [[Hand,:holding,:cereal]]},
        [:location,'*',:in_cupboard] => {:requires => [[Cupboard,:door,:open],[Hand,:holding,:nothing]]},
        [:location,:in_cupboard,'*'] => {:requires => [[Cupboard,:door,:open]]}
    }
)
Spoon = A_spoon.new('the spoon',
    :location => :on_counter,
    :transitions => {
        [:location,:on_counter,:in_cupboard] => {:takes => 1.second,:requires => [[Cupboard,:door,:open],[Hand,:holding,:nothing]]},
        [:location,:in_cupboard,:on_counter] => {:takes => 1.second,:requires => [[Cupboard,:door,:open],[Hand,:holding,:nothing]]}
    }
)
Bowl = A_bowl.new('the bowl',
    :location => :on_counter,
    :contains_cereal => true,
    :contains_milk => true,
    :transitions => {
        [:location,:on_counter,:in_cupboard] => {:takes => 1.second,:requires => [[Cupboard,:door,:open],[Hand,:holding,:nothing]]},
        [:location,:in_cupboard,:on_counter] => {:takes => 1.second,:requires => [[Cupboard,:door,:open],[Hand,:holding,:nothing]]},
        [:contains_milk,  false,true] => {:requires => [["Bowl",:location,:on_counter],[Milk,:location,:mid_air]]},
        [:contains_cereal,false,true] => {:requires => [["Bowl",:location,:on_counter],[Cereal,:location,:mid_air]]}
    }
)

make_it_so

