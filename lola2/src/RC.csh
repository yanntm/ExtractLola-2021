foreach i (INPUTS/*/)
	echo LAUNCHING REACHABILITY CARDINALITY $i
	date +"%T"
	rm -f ${i}ReachabilityCardinality.log
	./lola --conf=myconf ${i}model.pnml --formula=${i}ReachabilityCardinality.xml >& ${i}ReachabilityCardinality.log || true
	pkill -9 -f "sara.*ReachabilityCardinality" 
end
echo REACHABILITY CARDINALITY FINISHED
