#ifndef VERSION_H
#define VERSION_H

// Stage Timer Version Information
// Update these values with each significant change

#define VERSION_MAJOR 3
#define VERSION_MINOR 4
#define VERSION_PATCH 0

// Build metadata (optional)
#define VERSION_BUILD "20250104"  // YYYYMMDD format

// Version string for display
#define VERSION_STRING "3.4.0"
#define VERSION_FULL "Stage Timer v3.4.0"
// Helper macro to get version as single number for comparisons
#define VERSION_NUMBER ((VERSION_MAJOR * 10000) + (VERSION_MINOR * 100) + VERSION_PATCH)

#endif // VERSION_H