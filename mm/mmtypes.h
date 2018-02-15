#pragma once

// From engine.h
template< typename SentType_ >
class ProofException;
template< typename SentType_ >
struct ProofError;
template< typename SentType_ >
struct ProofTree;

// From library.h
class Library;
class LibraryImpl;
class Assertion;

// From proof.h
class Proof;
class CompressedProof;
class UncompressedProof;
template< typename SentType_ >
class ProofExecutor;
class ProofOperator;

// From toolbox.h
class Assertion;
class LibraryToolbox;
