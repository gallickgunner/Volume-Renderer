#ifndef TRANSFERFUNCTION_H
#define TRANSFERFUNCTION_H

//#include "elements/GradientBarWidget.h"
#include "elements/AlphaControlSplineWidget.h"

class TransferFunction
{
    public:
        TransferFunction();
        ~TransferFunction();

        void render();

    private:
        enum class DataScale{
            NORMALIZED_SCALE,
            MEDICAL_SCALE,
            ADAPTIVE_SCALE,
        };
        DataScale data_scale;
        //GradientBarWidget grad_bar;
        AlphaControlSplineWidget alpha_spline;
        int max_dataset_val;
        int min_medical_val;
        int convertValue(DataScale to_scale, int value);

};

#endif // TRANSFERFUNCTION_H
