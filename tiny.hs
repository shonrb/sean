{- tiny.hs
 - An emulator for an imaginary computer hardware
 -} 
import Data.Bits
import Data.Maybe
import Numeric (showHex)
import Text.Printf (printf)

-- A description of an operation that was executed
data OpInfo = OI 
    { opName      :: String
    , operand     :: Maybe Int
    , inputValue  :: Maybe Int
    , outputValue :: Maybe Int }

instance Show OpInfo where
    show o = printf "%s %s %s %s"
        (opName o)
        (showHexOrNone (operand o))
        (showHexOrNone (inputValue o))
        (showHexOrNone (outputValue o))
        where
            showHexOrNone :: Maybe Int -> String
            showHexOrNone (Just v) = showHex v ""
            showHexOrNone Nothing  = "-"

-- The internal state of the machine
data TMState = TM
    { regIP      :: Int -- Instruction pointer register
    , regLI      :: Int -- Loop index register
    , regFR      :: Int -- Flag Register
    , regAC      :: Int -- Accumulator Register
    , memory     :: [Int]
    , inQueue    :: [Int] 
    , outQueue   :: [Int]
    , thisOpInfo :: Maybe OpInfo   -- Equal to Nothing on first state
    , nextOpInfo :: Maybe OpInfo } -- Equal to Nothing on final state

instance Show TMState where
    show t = printf "%s %s %s %s  %s  %s"
        (hex regIP)
        (hex regLI)
        (hex regFR)
        (hex regAC)
        (foldr (\x y -> y ++ (showHex x "")) "" (reverse (memory t)))
        (showMaybeOpinfo (nextOpInfo t))
        where 
            hex field = showHex (field t) ""
            showMaybeOpinfo Nothing  = ""
            showMaybeOpinfo (Just opInfo) = show opInfo 

data Flag 
    = Carry 
    | Zero 
    | Overflow 
    | Halt

--------------------------------
-- Helper Functions
--------------------------------

-- Replaces list[index] with val, if index is within bounds.
replaceIndex :: [t] -> Int -> t -> [t]
replaceIndex list index val = map replaceIfFound [0..(length list - 1)]
    where replaceIfFound x = if x == index then val else list !! x

-- Like Prelude.takeWhile, but includes the 
-- item after the point where the condition fails.
takeWhilePlusOne :: (a -> Bool) -> [a] -> [a]
takeWhilePlusOne _ [] = []
takeWhilePlusOne f (h : t) = if f h then [h] ++ takeWhilePlusOne f t else [h]

u4Add :: Int -> Int -> Int
u4Add a b = (a + b) `mod` 16

baseOpInfo :: OpInfo
baseOpInfo = OI
    { opName      = "Error: Empty Opcode"
    , operand     = Nothing
    , inputValue  = Nothing
    , outputValue = Nothing }

--------------------------------
-- Flag Functions
--------------------------------

flagIndex :: Flag -> Int
flagIndex f = case f of
    Halt     -> 3
    Overflow -> 2
    Zero     -> 1
    Carry    -> 0

setFlag :: Flag -> Bool -> Int -> Int
setFlag f v r
    | v == True = r .|. asBin
    | otherwise = r .&. complement asBin
    where asBin = 1 `shiftL` (flagIndex f)

getFlag :: Flag -> Int -> Bool
getFlag f r = (r `shiftR` flagIndex f) .&. 1 == 1

--------------------------------
-- Operations
--------------------------------

-- Get the operand of the current operation, increment the instruction pointer,
-- and update the op info. Used to implement some operations which
-- work with addresses. (LDA, STA, LDL)
tmGetAddr :: TMState -> String -> (TMState, Int)
tmGetAddr tm op = (newTm, address)
    where 
        address   = memory tm !! (regIP tm `u4Add` 1)
        newOpInfo = baseOpInfo 
            { opName  = op
            , operand = Just address }
        newTm = tm
            { regIP      = regIP tm `u4Add` 2
            , thisOpInfo = Just newOpInfo }

-- Jumps to the address given as the operand.
-- If a condition is given, only jump if the zero flag is equal to it.
tmJump :: Maybe Bool -> String -> TMState -> TMState
tmJump cond op tm = case cond of
    (Just g) -> if g == getFlag Zero (regFR tm) then sucTm else failTm
    Nothing  -> sucTm
    where 
        (failTm, addr) = tmGetAddr tm op
        sucTm          = failTm { regIP = addr }

-- Applies an arithmetic operation to AC, detecting carry, overflow and zero
tmDoArith :: TMState -> String -> (Int -> Int) -> Maybe Int -> TMState
tmDoArith tm name func oper = tm
    { regIP = regIP tm `u4Add` opLen
    , regAC = newAC
    , regFR = 
        ( setFlag Zero     (newAC == 0) 
        . setFlag Carry    cOut 
        . setFlag Overflow (cIn /= cOut)
        ) (regFR tm)
    , thisOpInfo = Just baseOpInfo 
        { opName  = name
        , operand = oper } }
    where 
        opLen  = if isNothing oper then 1 else 2 -- Amount to increment IP
        oldAC  = regAC tm
        cIn    = getFlag Carry (regFR tm)
        res    = func oldAC + if cIn then 1 else 0
        cOut   = res >= 16
        newAC  = res `mod` 16

-- Sets the carry flag to the given value
tmSetCarryFlag :: Bool -> String -> TMState -> TMState
tmSetCarryFlag val op tm = tm
    { regIP      = regIP tm `u4Add` 1
    , regFR      = setFlag Carry val (regFR tm)
    , thisOpInfo = Just baseOpInfo { opName = op } }

-- Halts the program execution.
tmHLT :: TMState -> TMState
tmHLT tm = tm
    { regFR      = setFlag Halt True (regFR tm)
    , regIP      = regIP tm `u4Add` 1
    , thisOpInfo = Just baseOpInfo { opName = "HLT" } }

-- Stores the contents of the given address in the AC register.
tmLDA :: TMState -> TMState
tmLDA tm = ntm { regAC = memory ntm !! addr } 
    where (ntm, addr) = tmGetAddr tm "LDA"

-- Writes the AC register to the given address.
tmSTA :: TMState -> TMState
tmSTA tm = ntm { memory = replaceIndex (memory ntm) addr (regAC ntm) }
    where (ntm, addr) = tmGetAddr tm "STA"

-- Gets an input from the input queue and stores it in the AC registers.
-- Kills the program if the input queue is empty.
tmGET :: TMState -> TMState
tmGET tm
    | inQueue tm == [] = tm
        { regFR      = setFlag Halt True (regFR tm) 
        , thisOpInfo = Just baseOpInfo { opName = "GET" } }
    | otherwise = tm
        { regIP      = regIP tm `u4Add` 1
        , regAC      = input
        , inQueue    = drop 1 (inQueue tm)
        , thisOpInfo = Just baseOpInfo
            { opName     = "GET"
            , inputValue = Just input } }
    where input = inQueue tm !! 0

-- Writes the contents of AC to the output queue
tmPUT :: TMState -> TMState
tmPUT tm = tm
    { regIP      = regIP tm `u4Add` 1
    , outQueue   = outQueue tm ++ [regAC tm]
    , thisOpInfo = Just baseOpInfo
        { opName      = "PUT"
        , outputValue = Just (regAC tm) } }

-- Arithmetic add operation with carry
tmADC :: TMState -> TMState
tmADC tm = tmDoArith tm "ADC" (\x -> x + (memory tm !! address)) (Just address)
    where address = memory tm !! (regIP tm `u4Add` 1)

-- Subtracts 1 from the loop index, setting the zero flag if it is zero
tmDEL :: TMState -> TMState
tmDEL tm = tm
    { regIP      = regIP tm `u4Add` 1
    , regFR      = setFlag Zero (newLoopI == 0) (regFR tm)
    , regLI      = newLoopI
    , thisOpInfo = Just baseOpInfo { opName = "DEL" } }
    where
        loopI    = regLI tm 
        newLoopI = if loopI == 0 then 15 else loopI - 1

-- Loads the loop index from the given address.
tmLDL :: TMState -> TMState
tmLDL tm = ntm { regLI = memory ntm !! addr }
    where (ntm, addr) = tmGetAddr tm "LDL"

-- Flips the accumulator.
tmFLA :: TMState -> TMState
tmFLA tm = tm
    { regIP      = regIP tm `u4Add` 1
    , regAC      = newregAC
    , regFR      = setFlag Zero (newregAC == 0) (regFR tm) 
    , thisOpInfo = Just baseOpInfo { opName = "FLA" } }
    where newregAC = 15 - regAC tm

-- Perform one cycle of the machine by reading the opcode at
-- the instruction pointer.
tmStep :: TMState -> TMState
tmStep tm = case (memory tm !! regIP tm) of
    0  -> tmHLT tm
    1  -> tmJump Nothing      "JMP" tm
    2  -> tmJump (Just True)  "JZE" tm
    3  -> tmJump (Just False) "JNZ" tm
    4  -> tmLDA tm
    5  -> tmSTA tm
    6  -> tmGET tm
    7  -> tmPUT tm
    8  -> tmDoArith tm "ROL" (`shiftL` 1) Nothing
    9  -> tmDoArith tm "ROR" (`shiftR` 1) Nothing
    10 -> tmADC tm
    11 -> tmSetCarryFlag False "CCF" tm
    12 -> tmSetCarryFlag True  "SCF" tm
    13 -> tmDEL tm
    14 -> tmLDL tm
    15 -> tmFLA tm

-- Runs the machine until it halts, 
-- return a list of the state after each cycle in sequence.
tmToTrace :: TMState -> [TMState]
tmToTrace = staggerOpInfos . takeWhilePlusOne halted . iterate tmStep
    where
        halted = \t -> not (getFlag Halt (regFR t))
        -- Set each item's nextOpInfo in the list to the next item's thisOpInfo
        staggerOpInfos [] = []
        staggerOpInfos (tm : []) = [tm]
        staggerOpInfos (tm : rest) = 
            [tm { nextOpInfo = thisOpInfo (rest !! 0) }] ++ staggerOpInfos rest

main :: IO ()
main = do
    let tm = TM { 
        regIP      = 0, 
        regLI      = 0, 
        regFR      = 0, 
        regAC      = 0, 
        memory     = [6,5,7,6,10,15,5,10,1,0,11,10,15,2,9,9],
        inQueue    = [10, 3, 8, 11, 0, 1],
        outQueue   = [],
        thisOpInfo = Nothing,
        nextOpInfo = Nothing
    }
    putStrLn "I L F A  Memory----------  Action---"
    putStrLn "P I R C  0123456789abcdef  OPR & ? !"
    mapM_ print (tmToTrace tm)
