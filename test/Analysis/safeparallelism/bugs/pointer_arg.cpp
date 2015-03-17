// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=effect-inference %s -verify

void cluster_exec (float** cluster_centres /*[[asap::arg("Local, Global, Global")]]*/) 
{   // expected-warning{{Inferred Effect Summary for cluster_exec: [reads(rpl([rLOCAL],[])),writes(rpl([rGLOBAL],[]))]}}
    float* tmp_cluster_centres [[asap::arg("Local, Global")]] = nullptr;
    *cluster_centres = tmp_cluster_centres;
}
