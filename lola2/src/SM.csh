foreach i (INPUTS/*/)
	echo LAUNCHING STABLEMARKING $i
	date +"%T"
	rm -f ${i}StableMarking.log
	./lola --conf=myconf ${i}model.pnml --check=StableMarking >& ${i}StableMarking.log || true
end
echo GLOBAL PROPERTIES FINISHED
