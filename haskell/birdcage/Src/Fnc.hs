module Fnc (getFunction) where

import Exp

getFunction :: String -> Maybe Func
getFunction s = do
  act <- getAction s
  return $ Func 
    { name = s
    , action = act
    , argsGiven = [] }

getAction :: String -> Maybe FuncAction
getAction str = case str of
  "print"              -> Just fPrint
  "bluebird"           -> Just b
  "blackbird"          -> Just b1
  "bunting"            -> Just b2
  "becard"             -> Just b3
  "cardinal"           -> Just c
  "cardinal*"          -> Just c'
  "cardinal**"         -> Just c''
  "dove"               -> Just d
  "dickcissel"         -> Just d1
  "dovekies"           -> Just d2
  "eagle"              -> Just e
  "bald_eagle"         -> Just eb
  "finch"              -> Just f
  "finch*"             -> Just f'
  "finch**"            -> Just f''
  "goldfinch"          -> Just g
  "hummingbird"        -> Just h
  "idiot"              -> Just i
  "idiot*"             -> Just i'
  "idiot**"            -> Just i''
  "jay"                -> Just j
  "kestrel"            -> Just k
  "lark"               -> Just l
  "mockingbird"        -> Just m
  "double_mockingbird" -> Just m2
  "owl"                -> Just o
  "queer_bird"         -> Just q
  "quixotic_bird"      -> Just q1
  "quizzical_bird"     -> Just q2
  "quirky_bird"        -> Just q3
  "quacky_bird"        -> Just q4
  "robin"              -> Just r
  "robin*"             -> Just r'
  "robin**"            -> Just r''
  "starling"           -> Just s
  "thrush"             -> Just t
  "turing"             -> Just u
  "vireo"              -> Just v
  "vireo*"             -> Just v'
  "vireo**"            -> Just v''
  "warbler"            -> Just w
  "warbler*"           -> Just w'
  "warbler**"          -> Just w''
  "converse_warbler"   -> Just w1
  "why"                -> Just y
  "kite"               -> Just ki
  "konstant_mocker"    -> Just km
  _ -> Nothing 

fPrint :: FuncAction
fPrint = FuncAction 1 $ Prints . show . head

mk :: Int -> ([Term] -> Term) -> FuncAction
mk c f = FuncAction c $ Pure . f

(~) = Application
infixl ~

b  = mk 3 (\[a,b,c]         -> a~(b~c) )
b1 = mk 4 (\[a,b,c,d]       -> a~(b~c~d) )
b2 = mk 5 (\[a,b,c,d,e]     -> a~(b~c~d~e) ) 
b3 = mk 4 (\[a,b,c,d]       -> a~(b~(c~d)) )
c  = mk 3 (\[a,b,c]         -> a~c~b )
d  = mk 4 (\[a,b,c,d]       -> a~b~(c~d) )
d1 = mk 5 (\[a,b,c,d,e]     -> a~b~c~(d~e) )
d2 = mk 5 (\[a,b,c,d,e]     -> a~(b~c)~(d~e) )
e  = mk 5 (\[a,b,c,d,e]     -> a~b~(c~d~e) )
eb = mk 7 (\[a,b,c,d,e,f,g] -> a~(b~c~d)~(e~f~g) )
f  = mk 3 (\[a,b,c]         -> c~b~a )
g  = mk 4 (\[a,b,c,d]       -> a~d~(b~c) )
h  = mk 3 (\[a,b,c]         -> a~b~c~b )
i  = mk 1 (\[a]             -> a)
j  = mk 4 (\[a,b,c,d]       -> a~b~(a~d~c) )
k  = mk 2 (\[a,b]           -> a)
l  = mk 2 (\[a,b]           -> a~(b~b) )
m  = mk 1 (\[a]             -> a~a )
m2 = mk 2 (\[a,b]           -> a~b~(a~b) )
o  = mk 2 (\[a,b]           -> b~(a~b) )
q  = mk 3 (\[a,b,c]         -> b~(a~c) )
q1 = mk 3 (\[a,b,c]         -> a~(c~b) )
q2 = mk 3 (\[a,b,c]         -> b~(c~a) )
q3 = mk 3 (\[a,b,c]         -> c~(a~b) )
q4 = mk 3 (\[a,b,c]         -> c~(b~a) )
r  = mk 3 (\[a,b,c]         -> b~c~a )
s  = mk 3 (\[a,b,c]         -> a~c~(b~c) )
t  = mk 2 (\[a,b]           -> b~a )
u  = mk 2 (\[a,b]           -> b~(a~a~b) )
v  = mk 3 (\[a,b,c]         -> c~a~b )
w  = mk 2 (\[a,b]           -> a~b~b )
w1 = mk 2 (\[a,b]           -> b~a~a )
i' = mk 2 (\[a,b]           -> a~b )
w' = mk 3 (\[a,b,c]         -> a~b~c~c )
c' = mk 4 (\[a,b,c,d]       -> a~b~d~c )
r' = mk 4 (\[a,b,c,d]       -> a~c~d~b )
f' = mk 4 (\[a,b,c,d]       -> a~d~c~b )
v' = mk 4 (\[a,b,c,d]       -> a~c~b~d )
i'' = mk 3 (\[a,b,c]        -> a~b~c )
w'' = mk 3 (\[a,b,c,d]      -> a~b~c~d~d )
c'' = mk 4 (\[a,b,c,d,e]    -> a~b~c~e~d )
r'' = mk 4 (\[a,b,c,d,e]    -> a~b~d~e~c )
f'' = mk 4 (\[a,b,c,d,e]    -> a~b~e~d~c )
v'' = mk 4 (\[a,b,c,d,e]    -> a~b~e~c~d )
ki  = mk 2 (\[a,b]          -> b)
km  = mk 2 (\[a,b]          -> b~b )
ckm = mk 2 (\[a,b]          -> a~a )
y   = mk 1 (\[a]            -> a~(Variable "why" ~ a) )

