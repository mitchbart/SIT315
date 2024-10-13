package main

import (
	"fmt"
	"os"
	"time"
)

// FrequencyTable generates a hashmap of all k-mers
func FrequencyTable(Text string, k int) map[string]int {
	freqMap := make(map[string]int) // init frequency map
	// Loop through entire genome
	for i := 0; i < len(Text)-k+1; i++ {
		pattern := Text[i : i+k] // isolate every nucleotide pattern
		_, ok := freqMap[pattern]
		if ok { // if value in freqMap, increment
			freqMap[pattern] += 1
		} else { // else, add to freqMap
			freqMap[pattern] = 1
		}
	}
	return freqMap
}

// FindClumps find nucleotide clumps in genome
func FindClumps(Genome string, k int, L int, t int) []string {
	patterns := make(map[string]int) // store patterns as a hashmap
	var result []string              // store result as slice
	n := len(Genome)
	// Loop through genome
	for i := 0; i <= n-L; i++ {
		window := Genome[i : i+L]            // set window
		freqMap := FrequencyTable(window, k) // get freqMap of window
		for key, value := range freqMap {    // check each item in freqMap
			if value >= t { // if value >= minimum frequency
				_, ok := patterns[key] // check if sequence exists in patterns
				if !ok {               // if it doesn't add pattern
					patterns[key] = 1
					result = append(result, key)
				}
			}
		}
	}
	return result
}

func main() {
	// Load E-coli genome
	c, err := os.ReadFile("E_coli.txt")
	if err != nil {
		fmt.Print(err)
	}
	EColiGenome := string(c)

	// Start timer
	start := time.Now()

	// Find clumps - the total number of clumps is printed
	fmt.Println(len(FindClumps(EColiGenome, 9, 500, 3)))

	// Stop timer and output result
	elapsed := time.Now().Sub(start)
	fmt.Printf("Sum function took %s", elapsed)
}
