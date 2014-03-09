#include <stdint.h>

#define FRACT_BITS 14 // Número de bits reservados para la parte decimal
#define F_CONST 16384 // Equivale a 2**14

/* Convierte un entero en un número equivalente en representación de punto fijo 17.14 */
#define INT_TO_FIXPOINT(p, q)   (((p)<<FRACT_BITS) / (q))
/* Convierte un número en representación de punto fijo 17.14 a un entero truncando su parte decimal */
#define FIXPOINT_TO_INT(x)      ((x) / F_CONST)
/* Multiplica dos números en representación de punto fijo 17.14 */
#define MULT_FP(x, y)        ((((int64_t) x) * (y)) / F_CONST)

#define MULT_FP_INT(x,i)      MULT_FP(x, INT_TO_FIXPOINT(i,1))

/* Divide dos números en representación de punto fijo 17.14*/
#define DIV_FP(x, y)         ( ( ((int64_t) x) * F_CONST ) / (y) )

#define DIV_FP_INT(x, i)    DIV_FP(x, INT_TO_FIXPOINT(i,1))

#define ADD_FP(x, y)        ((x) + (y))

#define ADD_FP_INT(x, i)    ADD_FP(x, INT_TO_FIXPOINT(i, 1))

#define SUB_FP(x, y)        ((x) - (y))

#define SUB_INT_FP(x, y)    SUB_FP(INT_TO_FIXPOINT(x, 1), y)

#define SUB_FP_INT(x, y)    SUB_FP(x, INT_TO_FIXPOINT(y, 1))

#define ROUND_FP_TO_INT(x)     ((x)>=0 ? (x + (F_CONST>>1))>>FRACT_BITS : (x - (F_CONST>>1))>>FRACT_BITS)

#define TO_INT_TRUNC(x)         ((x)/F_CONST)