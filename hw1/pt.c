#include <math.h>
#include <stdio.h>
#include "os.h"

#define NUM_LEVELS 5
#define ADDRESS_IN_LEVEL 512
#define BITS_OF_ADDRESS_PER_LEVEL 9
#define OFFSET_BITS 12
#define OFFSET_MASK 0xfff

/*
Calculations:
  4KB = 4096B = Number of bytes in page frame
  64bit = 8B = Number of bits in each address
  4096 / 8 = 512 = 2^9 = Number of addresses that can fit in a page
  45bit = number of effective bits in a vpn
  45/9 = 5 = number of level needed to represent all the effective bits in a vpn
*/

/*
  comvert a VPN to an array of 5 ints where each one represent the index in the respected level
*/
void vpn_to_indecies_array(uint64_t vpn, uint64_t* array)
{
  uint64_t mask = pow(2, BITS_OF_ADDRESS_PER_LEVEL) -1; // set lowest 9 bits to 1
  for (int i=0; i < NUM_LEVELS; i++)
  {
    array[NUM_LEVELS - i - 1] = vpn & mask;
    vpn >>= BITS_OF_ADDRESS_PER_LEVEL;
  }
}

/* checks if the valid bit is equal to 1 */ 
uint64_t is_valid(const uint64_t shifted_page_number)
{
  return shifted_page_number & 1;
}

/* 
  sets the valid bit to 1 
  shifted_page_number should be a physical page number shifted left to clear offst bits
  (This also represent the first address of the page)
*/ 
uint64_t set_valid(const uint64_t shifted_page_number)
{
  return shifted_page_number | 1;
}

/* 
  shift the page number to represent a physical address
  then sets it to valid
*/
uint64_t ppn_to_address(uint64_t ppn)
{
  return set_valid(ppn << OFFSET_BITS);
}

uint64_t clear_offset_bits(uint64_t address)
{
  return address & ~OFFSET_MASK;
}

/*
  returns a pointer to the beginning of the physical page that contains the given address
*/
uint64_t* get_page_pointer(uint64_t physical_address)
{
  return phys_to_virt(clear_offset_bits(physical_address));
}

/*
  pt - physical address of the root of the page table
  vpn - the virtual page number the caller wants to update
  ppn - the physical address that the vpn should map to (if equals NO_MAPPTING, destroy the current mapping)
*/
void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn)
{
  uint64_t index[NUM_LEVELS];
  uint64_t *page_ptr;
  uint64_t next_phys = pt;
  
  vpn_to_indecies_array(vpn, index);
  
  // we do it only num_levels -1 we are already at level 1
  for (uint16_t i = 0; i < NUM_LEVELS - 1; i++)
  {
    page_ptr = get_page_pointer(next_phys);
    next_phys = page_ptr[index[i]];
    if (!is_valid(next_phys))
    {
      uint64_t page_number = alloc_page_frame();
      next_phys = ppn_to_address(page_number);
      page_ptr[index[i]] = next_phys;
    }
  }

  page_ptr = get_page_pointer(next_phys);
  // set new value
  if (ppn == NO_MAPPING)
  {
    page_ptr[index[NUM_LEVELS - 1]] &= 0;
  } else {
    page_ptr[index[NUM_LEVELS - 1]] = ppn_to_address(ppn);
  }
}

uint64_t page_table_query(uint64_t pt, uint64_t vpn)
{
  uint64_t index[NUM_LEVELS];
  uint64_t *page_ptr;
  uint64_t next_phys = pt;
  vpn_to_indecies_array(vpn, index);
  
  // we do it num_levels, since we want to get the actual mapping at the end
  for (uint16_t i = 0; i < NUM_LEVELS; i++)
  {
    page_ptr = get_page_pointer(next_phys);
    next_phys = page_ptr[index[i]];
    if (!is_valid(next_phys))
    {
      return NO_MAPPING;
    }
  }
  // shift the addres right to turn it back to page number
  return next_phys >> OFFSET_BITS;
}
