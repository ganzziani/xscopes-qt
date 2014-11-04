#include "customcolors.h"

CustomColors::CustomColors(){
    m_buttonsMap.clear();

    m_buttonsMap.insert("CH1",&ch1);
    m_buttonsMap.insert("CH1 REF",&ch1ref);
    m_buttonsMap.insert("CH1 FFT",&ch1fft);

    m_buttonsMap.insert("CH2",&ch2);
    m_buttonsMap.insert("CH2 REF",&ch2ref);
    m_buttonsMap.insert("CH2 FFT",&ch2fft);

    m_buttonsMap.insert("GRID",&grid);
    m_buttonsMap.insert("AXES",&axes);

    for(int i = 0; i < 8; i++){
        m_buttonsMap.insert("BIT "+QString::number(i),&bit[i]);
        m_buttonsMap.insert("BIT "+QString::number(i)+" REF",&bit[i]);
    }
    m_buttonsMap.insert("LABELS",&label);
}

QColor CustomColors::colorAt(QString from){
    if(from == "BACKGROUND")
        return background.color();
    return *m_buttonsMap.value(from);
}


void CustomColors::setColor(QString from, QColor new_color){
    if(from == "BACKGROUND")
        background.setColor(new_color);
    else
        *(m_buttonsMap.value(from)) = new_color;
}
