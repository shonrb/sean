{-# LANGUAGE RecordWildCards #-}
module Exp where

import Data.List (intercalate)

data CallResult 
  = Pure Term
  | Prints String

data FuncAction = FuncAction
  { argsNeeded :: Int
  , op :: [Term] -> CallResult
  }

data Func = Func
  { name :: String
  , action :: FuncAction
  , argsGiven :: [Term]
  }

data Term
  = Variable String
  | StringLit String
  | Application Term Term
  | Function Func
  | NullTerm

data Expression 
  = Assignment String Term
  | TermOnly Term

instance Show Func where
  show f@Func{..} = name ++ case argsGiven of
    [] -> []
    a  -> intercalate " " . map (showTerm True) $ a

instance Show Term where
  show s = case s of
    StringLit s -> s
    _           -> showTerm False s

instance Show Expression where
  show x = case x of
    Assignment s t -> s ++ " = " ++ show t
    TermOnly t     -> show t

showTerm :: Bool -> Term -> String
showTerm bracket x = case x of
  Variable s      -> s
  StringLit s     -> s 
  NullTerm        -> "()"
  Function f      -> show f
  Application a b ->
    if bracket 
    then "(" ++ app ++ ")"
    else app
    where app = showTerm False a ++ " " ++ showTerm True b

applyFunc :: Func -> Term -> CallResult
applyFunc f@Func{..} term = 
  if   length have == argsNeeded action
  then op action have
  else Pure $ Function $ f { argsGiven = have } 
  where have = argsGiven ++ [term]

