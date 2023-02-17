foreach i (INPUTS/*/)
	echo LAUNCHING REACHABILITY FIREABILITY $i
	date +"%T"
	rm -f ${i}ReachabilityFireability.log
	./lola --conf=myconf ${i}model.pnml --formula=${i}ReachabilityFireability.xml >& ${i}ReachabilityFireability.log || true
	pkill -9 -f "sara.*ReachabilityFireability" 
end
echo REACHABILITY FIREABLITY FINISHED
