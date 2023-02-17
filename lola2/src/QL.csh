foreach i (INPUTS_2020/*/)
	echo LAUNCHING QUASILIVE $i
	date +"%T"
	rm -f ${i}QuasiLiveness.log
	./lola --conf=myconf --compare ${i}model.pnml --check=QuasiLiveness >& ${i}QuasiLiveness.log || true
end
echo QUASILIVE FINISHED
