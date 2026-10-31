#include "NeuralNetworkClassifierLabelTest.h"

#include <string>
#include <vector>

#include "Vision/NeuralNetworkClassifierLabel.h"

bool neural_network_classifier_label_test()
{
    const std::vector<std::string> no_labels;
    const std::vector<std::string> partial_labels { "Cat" };
    const std::vector<std::string> complete_labels { "Cat", "Dog" };

    return MaaNS::VisionNS::nn_classifier_label(no_labels, 0) == "Unknown_0"
           && MaaNS::VisionNS::nn_classifier_label(partial_labels, 0) == "Cat"
           && MaaNS::VisionNS::nn_classifier_label(partial_labels, 1) == "Unknown_1"
           && MaaNS::VisionNS::nn_classifier_label(complete_labels, 1) == "Dog";
}
