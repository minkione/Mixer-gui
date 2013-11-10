#include "fader.h"
#include "ui_fader.h"

Fader::Fader(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Fader)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose, true);

    for(int i; i<15; i++){
        ui->outputSelect->addItem(QString::number(i));
    }

    connect(ui->enableBtn, SIGNAL(stateChanged(int)), this, SLOT(controlChanged(int)));
//    connect(ui->checkBox_5, SIGNAL(stateChanged(int)), this, SLOT(faderPreview(int)));

    connect(ui->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(controlChanged(int)));
    connect(ui->horizontalSlider, SIGNAL(sliderReleased()), this, SLOT(controlClicked()));

    connect(ui->redButtons, SIGNAL(buttonReleased(int)), this, SLOT(controlChanged(int)));
    connect(ui->yellowButtons, SIGNAL(buttonReleased(int)), this, SLOT(controlChanged(int)));

    connect(ui->fadeButton, SIGNAL(clicked()), this, SLOT(controlClicked()));

}

Fader::~Fader()
{
    run=0;
    delete ui;
}

void Fader::keyPressEvent(QKeyEvent* event) {
    fprintf(stderr,"key %d\n", event->key());
    if (event->key()>47 and event->key()<56){
//obmedz
        ui->yelFrame->findChild<QRadioButton*>(QString("yel%1").arg(event->key()-48))->click();
    }
    if (event->key()==16777221){
        autofade();
    }

}

void Fader::start(){

    out=(uchar*)buffer->Open(ui->outputSelect->currentIndex());

    red=(uchar*)buffer->Open(0);
    yel=(uchar*)buffer->Open(0);

    rate=0;

    run=1;

    pthread_create(&thread, NULL, Fader::staticEntryPoint, this);
}

void Fader::controlClicked(){
    QWidget* sndr = (QWidget *)sender();
    QString name = sndr->objectName();
    fprintf (stderr,"%s sender\n",sndr->objectName().toStdString().c_str());

    if (name.contains("horizontalSlider") && rate==1){
        QString newRed = QString("red%1").arg(ui->yellowButtons->checkedButton()->objectName().at(3).digitValue());
        QString newYel = QString("yel%1").arg(ui->redButtons->checkedButton()->objectName().at(3).digitValue());

        ui->centralwidget->findChild<QRadioButton*>(newRed)->click();
        ui->horizontalSlider->setValue(0);
        ui->centralwidget->findChild<QRadioButton*>(newYel)->click();
    }

    if (name.contains("fadeButton")){
        autofade();
    }

}

void Fader::controlChanged(int value){
    QWidget* sndr = (QWidget *)sender();
    QString name = sndr->objectName();
    fprintf (stderr,"sender:%s value:%d\n",sndr->objectName().toStdString().c_str(), value);

    if(name.contains("horizontalSlider")){

        rate = ((float)value)/100;

        if(!ui->horizontalSlider->isSliderDown() && value==100){
            QString newRed = QString("red%1").arg(ui->yellowButtons->checkedButton()->objectName().at(3).digitValue());
            QString newYel = QString("yel%1").arg(ui->redButtons->checkedButton()->objectName().at(3).digitValue());

            ui->centralwidget->findChild<QRadioButton*>(newRed)->click();
            ui->horizontalSlider->setValue(0);
            ui->centralwidget->findChild<QRadioButton*>(newYel)->click();
        }
    }

    if(name.contains("enableBtn")){
        //dorobit stop
        start();
    }

    if(name.contains("yellowButtons")){
        //fprintf (stderr,"checked %d\n",ui->buttonGroup_2->checkedButton()->objectName().at(3).digitValue());
        setYellow(ui->yellowButtons->checkedButton()->objectName().at(3).digitValue());
    }

    if(name.contains("redButtons")){
        //fprintf (stderr,"checked %d\n",ui->buttonGroup_2->checkedButton()->objectName().at(3).digitValue());
        setRed(ui->redButtons->checkedButton()->objectName().at(3).digitValue());
    }

}

class HelloWorldTask : public QRunnable
{
    Fader* rodic;
    int usleeptime;

public: void Init(Fader* rodi, int usleeptim){
        rodic = rodi;
        usleeptime = usleeptim;
    }

    void run()
    {
        for (int value=0; value<=100; value++){
            usleep(usleeptime*1000);
            rodic->setFader(value);
        }
        fprintf(stderr,"runable\n");
    }
};

void Fader::autofade(){
    int leng = ui->fadeLength->value();



    HelloWorldTask *bezec = new HelloWorldTask();
    bezec->Init(this, leng/100);
    // QThreadPool takes ownership and deletes 'hello' automatically
    QThreadPool::globalInstance()->start(bezec);

}

void Fader::setYellow(int next){
    fprintf(stderr,"setting %d\n",next);
    yel = (uchar*) buffer->Open(next);
}

void Fader::setRed(int next){
    fprintf(stderr,"setting %d\n",next);
    red = (uchar*) buffer->Open(next);

}

void Fader::Init(Buffer* buf)
{
    buffer = buf;

}

void * Fader::staticEntryPoint(void * c)
{
    ((Fader *) c)->Thread();
    return NULL;
}

void Fader::setFader(int rat){
    ui->horizontalSlider->setValue(rat);

}

void Fader::Thread(){

    int y;
    int i;
    pos = buffer->frame[0];
    float rateSup;

    while(run){

        if ((pos!=buffer->frame[0]) || (buffer->frame[0]==-1)){
            //fprintf(stderr,"rate %f\n",rate);
            pos=buffer->frame[0];

            if (rate==0) {

                memcpy(out,red,buffer->buf_len);

            } else {
                rateSup=1-rate;

                for(i=0; i<buffer->buf_len;i++){

                    out[i] =(rateSup*(float)red[i] + rate*(float)yel[i]) ;
                }

            }

        } else {
            usleep(10*1000);
        }
    }

}
