use std::sync::Arc;

use anyhow::Result;

use esp_idf_hal::gpio::{Gpio18, Gpio19, Gpio3, Gpio4, Gpio5, Output};
use esp_idf_hal::ledc::config::TimerConfig;
use esp_idf_hal::ledc::{Channel, Timer};
use esp_idf_hal::ledc::{CHANNEL0, CHANNEL1, CHANNEL2, CHANNEL3, CHANNEL4, TIMER0};
use esp_idf_hal::prelude::*;

pub(crate) struct Led {
    r: Channel<CHANNEL0, TIMER0, Arc<Timer<TIMER0>>, Gpio3<Output>>,
    g: Channel<CHANNEL1, TIMER0, Arc<Timer<TIMER0>>, Gpio4<Output>>,
    b: Channel<CHANNEL2, TIMER0, Arc<Timer<TIMER0>>, Gpio5<Output>>,
    c: Channel<CHANNEL3, TIMER0, Arc<Timer<TIMER0>>, Gpio18<Output>>,
    w: Channel<CHANNEL4, TIMER0, Arc<Timer<TIMER0>>, Gpio19<Output>>,
    max_duty: u32,
}

impl Led {
    pub(crate) fn new(
        ch_r: CHANNEL0,
        io_r: Gpio3<Output>,
        ch_g: CHANNEL1,
        io_g: Gpio4<Output>,
        ch_b: CHANNEL2,
        io_b: Gpio5<Output>,
        ch_c: CHANNEL3,
        io_c: Gpio18<Output>,
        ch_w: CHANNEL4,
        io_w: Gpio19<Output>,
        t0: TIMER0,
    ) -> Result<Self> {
        let config = TimerConfig::default().frequency(25.kHz().into());
        let timer = Arc::from(Timer::new(t0, &config)?);
        let r = Channel::new(ch_r, timer.clone(), io_r)?;
        let g = Channel::new(ch_g, timer.clone(), io_g)?;
        let b = Channel::new(ch_b, timer.clone(), io_b)?;
        let c = Channel::new(ch_c, timer.clone(), io_c)?;
        let w = Channel::new(ch_w, timer, io_w)?;
        let max_duty = r.get_max_duty();
        let mut led = Self {
            r,
            g,
            b,
            c,
            w,
            max_duty,
        };

        led.set_brightness(0, 0)?;
        led.set_color(0, 0, 0)?;
        Ok(led)
    }

    pub(crate) fn set_brightness(&mut self, c: u8, w: u8) -> Result<()> {
        self.c
            .set_duty((self.max_duty * c as u32) / u8::MAX as u32)?;
        self.w
            .set_duty((self.max_duty * w as u32) / u8::MAX as u32)?;
        Ok(())
    }

    pub(crate) fn set_color(&mut self, r: u8, g: u8, b: u8) -> Result<()> {
        self.r
            .set_duty((self.max_duty * r as u32) / u8::MAX as u32)?;
        self.g
            .set_duty((self.max_duty * g as u32) / u8::MAX as u32)?;
        self.b
            .set_duty((self.max_duty * b as u32) / u8::MAX as u32)?;
        Ok(())
    }
}
