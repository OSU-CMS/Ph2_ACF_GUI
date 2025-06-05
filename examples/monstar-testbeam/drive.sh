POS=1

while true; do
    echo "Now driving to pos${POS}"
    eval "pos${POS}"
    if [ $POS -lt 16 ]; then
        POS=$(expr $POS + 1)
    else
        POS=1
    fi

    for i in {1..10}; do
        hor=($(itkpix_horizontal position_user | grep -v 'enc' | tr -d '\n'))
        ver=($(itkpix_vertical position_user | grep -v 'enc' | tr -d '\n'))
        echo at ${hor[1]} ${ver[1]}
        sleep 0.2
    done
    echo "Press enter after next spill to drive to next position..."
    read
done
