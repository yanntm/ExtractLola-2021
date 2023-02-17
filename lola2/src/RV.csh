foreach i (INPUTS/*/)
	echo LAUNCHING REVERSIBILITY $i
	date +"%T"
	rm -f ${i}Reversibility.log
	./lola --conf=myconf ${i}model.pnml --check=reversibility >& ${i}Reversibility.log || true
end
echo GLOBAL PROPERTIES FINISHED
