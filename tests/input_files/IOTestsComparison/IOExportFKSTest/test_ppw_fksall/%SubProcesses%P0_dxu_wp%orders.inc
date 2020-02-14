C     The orders to be integrated for the Born and at NLO
      INTEGER NSPLITORDERS
      PARAMETER (NSPLITORDERS=2)
      CHARACTER*3 ORDERNAMES(NSPLITORDERS)
      DATA ORDERNAMES / 'QCD', 'QED' /
      INTEGER BORN_ORDERS(NSPLITORDERS), NLO_ORDERS(NSPLITORDERS)
C     the order of the coupling orders is QCD, QED
      DATA BORN_ORDERS / 0, 2 /
      DATA NLO_ORDERS / 2, 2 /
C     The position of the QCD /QED orders in the array
      INTEGER QCD_POS, QED_POS
C     if = -1, then it is not in the split_orders
      PARAMETER (QCD_POS = 1)
      PARAMETER (QED_POS = 2)
C     this is to keep track of the various coupling combinations
C      entering each ME
      INTEGER AMP_SPLIT_SIZE, AMP_SPLIT_SIZE_BORN
      PARAMETER (AMP_SPLIT_SIZE = 2)
      PARAMETER (AMP_SPLIT_SIZE_BORN = 1)  ! the first entries in amp_split are for the born
      DOUBLE PRECISION AMP_SPLIT(AMP_SPLIT_SIZE)
      DOUBLE COMPLEX AMP_SPLIT_CNT(AMP_SPLIT_SIZE,2,NSPLITORDERS)
      COMMON /TO_AMP_SPLIT/AMP_SPLIT, AMP_SPLIT_CNT
