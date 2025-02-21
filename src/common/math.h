#ifndef MATH_H_
#define MATH_H_
#ifdef __cplusplus
extern "C" {
#endif

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define CONSTRAIN(val, low, high) ((val) < (low) ? (low) : ((val) > (high) ? (high) : (val)))

#ifdef __cplusplus
}
#endif
#endif /* MATH_H_ */