package com.google.android.exoplayer2.ext.hevc;

import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.CoreMatchers.is;
import static org.junit.Assert.*;

/**
 * Created by leoliu on 2017/11/21.
 */
@RunWith(AndroidJUnit4.class)
public class CalcTest {

    @Test
    public void addTwoNumbers() throws Exception {
        Calc calc = new Calc();
        assertThat(9 ,is(equalTo(calc.add(5,4))));
    }

}