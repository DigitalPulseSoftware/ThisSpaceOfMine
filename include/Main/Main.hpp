// Copyright (C) 2023 Jérôme Leclercq
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_MAIN_HPP
#define TSOM_MAIN_HPP

extern int TSOMEntry(int argc, char* argv[], int(*mainFunc)(int argc, char* argv[]));

#define TSOMMain(FuncName) int main(int argc, char* argv[]) \
{ \
	return TSOMEntry(argc, argv, &FuncName);\
}

#include <Main/Main.inl>

#endif
