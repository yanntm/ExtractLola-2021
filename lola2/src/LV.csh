foreach i (INPUTS_2020/*/)
	echo LAUNCHING LIVENESS $i
	date +"%T"
	rm -f ${i}Liveness.log
	./lola --conf=myconf --compare --formula=${i}UpperBounds.xml ${i}model.pnml --check=Liveness >& ${i}Liveness.log || true
end
echo LIVENESS FINISHED
