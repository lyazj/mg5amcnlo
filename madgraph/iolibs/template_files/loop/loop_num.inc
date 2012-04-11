      SUBROUTINE MPLOOPNUM(Q,RES)
C  
C ARGUMENTS 
C  
      INCLUDE 'cts_mprec.h'
      INCLUDE 'cts_mpc.h'                                             , INTENT(IN), DIMENSION(0:3) :: Q
      INCLUDE 'cts_mpc.h'                                             , INTENT(OUT) :: RES
C  
C LOCAL VARIABLES 
C
      COMPLEX*16 DRES
      COMPLEX*16 DQ(0:3)
      INTEGER I
C ----------
C BEGIN CODE
C ----------
      DO I=0,3
        DQ(I) = Q(I)
      ENDDO

      CALL LOOPNUM(DQ,DRES)
      RES=DRES

      END

C THE CORE SUBROUTINE CALLED BY CUTTOOLS WHICH CONTAINS THE HELAS CALLS BUILDING THE LOOP

      SUBROUTINE LOOPNUM(Q,RES)
C  
C CONSTANTS 
C  
      INTEGER    NEXTERNAL
      PARAMETER (NEXTERNAL=%(nexternal)d)
	  INTEGER    MAXLCOUPLINGS
      PARAMETER (MAXLCOUPLINGS=%(maxlcouplings)d)
	  INTEGER    NMAXLOOPWFS
      PARAMETER (NMAXLOOPWFS=(NEXTERNAL+1))
	  REAL*8     ZERO
	  PARAMETER (ZERO=0.D0)
C  
C ARGUMENTS 
C  
      COMPLEX*16 Q(0:3)
      COMPLEX*16 RES
C  
C LOCAL VARIABLES 
C
      COMPLEX*16 BUFF(12)
      COMPLEX*16 WL(20,NMAXLOOPWFS)
      INTEGER I
C
C GLOBAL VARIABLES
C
      COMPLEX*16 WE(20,NEXTERNAL)
	  COMPLEX*16 LC(MAXLCOUPLINGS)
	  REAL*8 ML(NEXTERNAL+1)	  
      INTEGER ID, SYMFACT
      COMMON/LOOP/WE,LC,ML,ID,SYMFACT
C ----------
C BEGIN CODE
C ----------
      RES=(0.D0,0.D0)
      %(loop_helas_calls)s
	  RES=RES/SYMFACT
      END