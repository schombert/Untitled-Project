#include "globalhelpers.h"
#include "reputation.h"



double update_reputation(double p_accurate, double p_innaccurate, double prior) noexcept {
	const double factor_a = p_accurate * prior;
	return factor_a / (factor_a + (p_innaccurate * (1.0 - prior)));
}

const double drift_percent = (1.0 - 1.0 / 16.0);
const double drift_percent_compliment = ((1.0 / 16.0) * 0.5);

double drift_reputation(double prior) noexcept {
	return prior * drift_percent + drift_percent_compliment;
}