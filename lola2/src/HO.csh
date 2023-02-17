foreach i (INPUTS/*/)
	echo LAUNCHING HOMESTATES $i
	date +"%T"
	rm -f ${i}HomeStates.log
	./lola --conf=myconf ${i}model.pnml --check=homestates >& ${i}HomeStates.log || true
end
echo GLOBAL PROPERTIES FINISHED
