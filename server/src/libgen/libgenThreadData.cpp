void neverCalled()
{
    ThreadData<bool> dummy1;
    dummy1.setValue( false );
    dummy1.getUpdatedValue();
    dummy1.getUpdatedValue( 1 );
    dummy1.getUpdatedValue( 0.1 );
    dummy1.updateValueSignal( true );
    dummy1.updateValueBroadcast( true );

    ThreadData<int> dummy2;
    dummy2.getValue();
    dummy2.setValue( 1 );
    dummy2.getUpdatedValue( 1 );
    dummy2.updateValueBroadcast( true );
}
