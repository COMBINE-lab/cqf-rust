#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

#[cfg(test)]
mod tests {
  use super::*;
  use std::ptr;
  #[test]
  fn basic_cqf() {
	    let mut qf:QF = QF{ runtimedata: ptr::null_mut(),
                          metadata: ptr::null_mut(),
                          blocks: ptr::null_mut() };
      let nslots = 256;
      let nhashbits = 8;
      unsafe {
          let r =  qf_malloc(&mut qf, nslots, nhashbits, 0, qf_hashmode::QF_HASH_INVERTIBLE, 0);
      }
	//qf_malloc(&qf, nslots, nhashbits, 0, QF_HASH_INVERTIBLE, 0)) {
  }
}
